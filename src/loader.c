// PTHREAD_CREATE_JOINABLE might be defined here
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include <smem.h>
#include <utils.h>
#include <log.h>
#include <harp.h>
#include <connection_pool.h>
#include <worker.h>
#include <options.h>

#include <loader.h>

pthread_cond_t  global_flags_cond;
pthread_mutex_t global_flags_mutex;
bool            global_reload_config_flag;
bool            global_terminate_flag;

inline void lock_flags_mutex() {
  int rc = pthread_mutex_lock(&global_flags_mutex);
  if(rc != 0) {
    errno = rc;
    logerror("lock_flags_mutex:pthread_mutex_lock");
    exit(EXIT_FAILURE);
  }
}

inline void unlock_flags_mutex() {
  int rc = pthread_mutex_unlock(&global_flags_mutex);
  if(rc != 0) {
    errno = rc;
    logerror("unlock_flags_mutex:pthread_mutex_unlock");
    exit(EXIT_FAILURE);
  }
}

void initialize_loader(loader_environment_t *loader_environment,
                       options_t *options) {
  int rc;

  loader_environment->options            = options;
  loader_environment->threads            = NULL;
  loader_environment->ports              = HARP_EMPTY_LIST;
  loader_environment->worker_environment = NULL;

  rc = pthread_cond_init(&global_flags_cond, NULL);
  if(rc != 0) {
    errno = rc;
    logerror("initialize_loader:pthread_cond_init");
    exit(EXIT_FAILURE);
  }

  rc = pthread_mutex_init(&global_flags_mutex, NULL);
  if(rc != 0) {
    errno = rc;
    logerror("initialize_loader:pthread_mutex_init");
    exit(EXIT_FAILURE);
  }

  global_reload_config_flag = false;
  global_terminate_flag     = false;
}

void deinitialize_loader() {
  int rc;

  rc = pthread_cond_destroy(&global_flags_cond);
  if(rc != 0) {
    errno = rc;
    logerror("deinitialize_loader:pthread_cond_destroy");
    exit(EXIT_FAILURE);
  }

  rc = pthread_mutex_destroy(&global_flags_mutex);
  if(rc != 0) {
    errno = rc;
    logerror("deinitialize_loader:pthread_mutex_destroy");
    exit(EXIT_FAILURE);
  }
}

void wait_for_flag_change() {
  lock_flags_mutex();

  int rc = pthread_cond_wait(&global_flags_cond, &global_flags_mutex);
  if(rc != 0) {
    errno = rc;
    logerror("wait_for_flag_change:pthread_cond_wait");
    exit(EXIT_FAILURE);
  }

  unlock_flags_mutex();
}

bool check_reload_config_flag() {
  bool val = global_reload_config_flag;
  global_reload_config_flag = false;
  return val;
}

bool check_terminate_flag() {
  bool val = global_terminate_flag;
  global_terminate_flag = false;
  return val;
}

/*
 * Signal handlers
 */

void sighup_handler(int sig) {
  int saved_errno = errno;
  lock_flags_mutex();

  global_reload_config_flag = true;
  int rc = pthread_cond_signal(&global_flags_cond);
  if(rc != 0) {
    errno = rc;
    // FIXME: Can't use logerror in a signal handler
    perror("sighup_handler:pthread_cond_signal");
  }

  unlock_flags_mutex();
  errno = saved_errno;
}

void sigterm_handler(int sig) {
  int saved_errno = errno;
  lock_flags_mutex();

  global_terminate_flag = true;
  int rc = pthread_cond_signal(&global_flags_cond);
  if(rc != 0) {
    errno = rc;
    // FIXME: Can't use logerror in a signal handler
    perror("sigterm_handler:pthread_cond_signal");
  }

  unlock_flags_mutex();
  errno = saved_errno;
}

/*
 * Actions
 */

void reload_configuration(loader_environment_t *loader_environment,
                          char *root_directory) {
  int rc, i;

  if(loader_environment->worker_environment == NULL) {
    logmsg(LOG_INFO, "Loading the configuration\n");
  } else {
    logmsg(LOG_INFO, "Reloading the configuration\n");
  }

  pthread_t *oldthreads         = loader_environment->threads;
  harp_list_t *oldports         = loader_environment->ports;
  worker_environment_t *oldwenv = loader_environment->worker_environment;

  harp_list_t *configs =
    harp_read_configs(loader_environment->options->config_path, root_directory);
  if(configs == NULL) {
    logmsg(LOG_ERR, "Couldn't read the configuration at %s: %s\n",
           loader_environment->options->config_path, harp_strerror(harp_errno));
    return;
  }

  harp_list_t *ports = harp_get_ports(configs);
  harp_list_t *oldcpool = oldwenv == NULL ? NULL : oldwenv->connection_pool;
  harp_list_t *connection_pool = create_connection_pool(ports, oldcpool);

  // It's not too late to cancel at this point
  if(connection_pool == NULL) {
    harp_free_list(configs, (harp_free_function_t*)&harp_free_config);
    harp_free_list(ports, NULL);
    return;
  }

  worker_environment_t *wenv =
    make_worker_environment(connection_pool, configs);

  // No lock because the state of valid is false at creation and can only
  // be changed to false afterwards anyway.
  if(oldwenv != NULL) {
    oldwenv->valid = false;
  }

#define EXIT_WITH_PTHREAD_ERROR(name) do {              \
    if(rc != 0) {                                       \
      errno = rc;                                       \
      logerror(name);                                   \
      destroy_worker_environment(wenv, oldports);       \
      harp_free_list(ports, NULL);                     \
      exit(EXIT_FAILURE);                               \
    }                                                   \
  } while(0)

  int thread_number  = loader_environment->options->thread_number;
  pthread_t *threads = (pthread_t*)smalloc(thread_number * sizeof(pthread_t));

  pthread_attr_t attr;
  rc = pthread_attr_init(&attr);
  EXIT_WITH_PTHREAD_ERROR("reload_configuration:pthread_attr_init");
  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  EXIT_WITH_PTHREAD_ERROR("reload_configuration:pthread_attr_setdetachstate");

  for(i = 0; i < thread_number; i++) {
    rc = pthread_create(&threads[i], &attr, (void*)&worker, wenv);
    if(rc != 0) {
      logerror("reload_configuration:pthread_create");
      pthread_attr_destroy(&attr);
      destroy_worker_environment(wenv, oldports);
      exit(EXIT_FAILURE);
    }
  }

  rc = pthread_attr_destroy(&attr);
  EXIT_WITH_PTHREAD_ERROR("reload_configuration:pthread_attr_destroy");

  if(oldthreads != NULL) {
    for(i = 0; i < thread_number; i++) {
      rc = pthread_join(oldthreads[i], NULL);
      EXIT_WITH_PTHREAD_ERROR("reload_configuration:pthread_join");
    }
  }

  free(oldthreads);
  harp_free_list(oldports, NULL);
  destroy_worker_environment(oldwenv, ports);

  loader_environment->threads            = threads;
  loader_environment->ports              = ports;
  loader_environment->worker_environment = wenv;
}

void terminate(loader_environment_t *loader_environment) {
  logmsg(LOG_NOTICE, "Terminating gracefully\n");

  // No lock because the state of valid is false at creation and can only
  // be changed to false afterwards anyway.
  if(loader_environment->worker_environment != NULL) {
    loader_environment->worker_environment->valid = false;
  }

  int thread_number  = loader_environment->options->thread_number;
  pthread_t *threads = loader_environment->threads;
  if(threads != NULL) {
    int i;
    for(i = 0; i < thread_number; i++) {
      int rc = pthread_join(threads[i], NULL);
      if(rc != 0) {
        errno = rc;
        logerror("terminate:pthread_join");
        exit(EXIT_FAILURE);
      }
    }
    free(threads);
  }

  destroy_worker_environment(loader_environment->worker_environment, NULL);
  harp_free_list(loader_environment->ports, NULL);
}
