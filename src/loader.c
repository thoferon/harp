#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include <memory.h>
#include <log.h>
#include <list.h>
#include <primp_config.h>
#include <connection_pool.h>
#include <worker.h>
#include <options.h>

#include <loader.h>

#define THREAD_NUMBER 8

pthread_cond_t  global_reload_config_cond;
pthread_mutex_t global_reload_config_mutex;
bool            global_reload_config_flag;

inline void lock_reload_config_mutex() {
  int rc = pthread_mutex_lock(&global_reload_config_mutex);
  if(rc != 0) {
    logerror("lock_reload_config_mutex:pthread_mutex_lock");
    exit(EXIT_FAILURE);
  }
}

inline void unlock_reload_config_mutex() {
  int rc = pthread_mutex_unlock(&global_reload_config_mutex);
  if(rc != 0) {
    logerror("unlock_reload_config_mutex:pthread_mutex_unlock");
    exit(EXIT_FAILURE);
  }
}

void initialize_loader(loader_environment_t *loader_environment,
                       options_t *options) {
  int rc;

  loader_environment->options            = options;
  loader_environment->threads            = NULL;
  loader_environment->ports              = EMPTY_LIST;
  loader_environment->worker_environment = NULL;

  rc = pthread_cond_init(&global_reload_config_cond, NULL);
  if(rc != 0) {
    logerror("initialize_loader:pthread_cond_init");
    exit(EXIT_FAILURE);
  }

  rc = pthread_mutex_init(&global_reload_config_mutex, NULL);
  if(rc != 0) {
    logerror("initialize_loader:pthread_mutex_init");
    exit(EXIT_FAILURE);
  }

  global_reload_config_flag = false;
}

void deinitialize_loader() {
  int rc;

  rc = pthread_cond_destroy(&global_reload_config_cond);
  if(rc != 0) {
    logerror("deinitialize_loader:pthread_cond_destroy");
    exit(EXIT_FAILURE);
  }

  rc = pthread_mutex_destroy(&global_reload_config_mutex);
  if(rc != 0) {
    logerror("deinitialize_loader:pthread_mutex_destroy");
    exit(EXIT_FAILURE);
  }
}

void wait_for_reload_config() {
  lock_reload_config_mutex();

  int rc = pthread_cond_wait(&global_reload_config_cond,
                             &global_reload_config_mutex);
  if(rc != 0) {
    logerror("wait_for_reload_config:pthread_cond_wait");
    exit(EXIT_FAILURE);
  }

  unlock_reload_config_mutex();
}

void sigusr1_handler(int sig) {
  int saved_errno = errno;

  lock_reload_config_mutex();

  global_reload_config_flag = true;
  int rc = pthread_cond_signal(&global_reload_config_cond);
  if(rc != 0) {
    // FIXME: Can't use logerror in a signal handler
    perror("sigusr1_handler:pthread_cond_signal");
  }

  unlock_reload_config_mutex();

  errno = saved_errno;
}

void reload_configuration(loader_environment_t *loader_environment) {
  int rc, i;

  if(loader_environment->worker_environment == NULL) {
    logmsg(LOG_INFO, "Loading the configuration\n");
  } else {
    logmsg(LOG_INFO, "Reloading the configuration\n");
  }

  pthread_t *oldthreads         = loader_environment->threads;
  list_t *oldports              = loader_environment->ports;
  worker_environment_t *oldwenv = loader_environment->worker_environment;

  list_t *configs = read_configs(loader_environment->options->config_path);
  if(configs == NULL) {
    logmsg(LOG_ERR, "Couldn't read the configuration at %s",
           loader_environment->options->config_path);
    return;
  }

  list_t *ports           = get_ports(configs);
  list_t *oldcpool        = oldwenv == NULL ? NULL : oldwenv->connection_pool;
  list_t *connection_pool = create_connection_pool(ports, oldcpool);
  free_list(ports, NULL);

  // It's not too late to cancel at this point
  if(connection_pool == NULL) {
    return;
  }

  worker_environment_t *wenv =
    make_worker_environment(connection_pool, configs);

  // No lock because the state of valid is false at creation and can only
  // be changed to false afterwards anyway. Also, this is a signal handler
  // so we can't wait for another thread to release the lock.
  if(oldwenv != NULL) {
    oldwenv->valid = false;
  }

#define EXITWITHERROR(name) do {                        \
    if(rc != 0) {                                       \
      logerror(name);                                   \
      destroy_worker_environment(wenv, oldports);       \
      exit(EXIT_FAILURE);                               \
    }                                                   \
  } while(0)

  pthread_t *threads = (pthread_t*)smalloc(THREAD_NUMBER * sizeof(pthread_t));

  pthread_attr_t attr;
  rc = pthread_attr_init(&attr);
  EXITWITHERROR("sigusr1_handler:pthread_attr_init");
  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  EXITWITHERROR("sigusr1_handler:pthread_attr_setdetachstate");

  for(i = 0; i < THREAD_NUMBER; i++) {
    rc = pthread_create(&threads[i], NULL, (void*)&worker, wenv);
    if(rc != 0) {
      logerror("sigusr1_handler:pthread_create");
      pthread_attr_destroy(&attr);
      destroy_worker_environment(wenv, oldports);
      exit(EXIT_FAILURE);
    }
  }

  rc = pthread_attr_destroy(&attr);
  EXITWITHERROR("sigusr1_handler:pthread_attr_destroy");

  if(oldthreads != NULL) {
    for(i = 0; i < THREAD_NUMBER; i++) {
      rc = pthread_join(oldthreads[i], NULL);
      EXITWITHERROR("sigusr1_handler:pthread_join");
    }
  }

  if(oldwenv != NULL) {
    destroy_worker_environment(oldwenv, ports);
  }

  loader_environment->threads            = threads;
  loader_environment->ports              = ports;
  loader_environment->worker_environment = wenv;
}
