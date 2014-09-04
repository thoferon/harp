#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include <memory.h>
#include <log.h>
#include <harp.h>
#include <request.h>
#include <harp_config/find.h>
#include <connection_pool.h>
#include <resolve.h>
#include <start.h>

#include <worker.h>

worker_environment_t *make_worker_environment(
    harp_list_t *connection_pool, harp_list_t *configs) {
  worker_environment_t *worker_environment =
    (worker_environment_t*)smalloc(sizeof(struct worker_environment));

  pthread_mutex_t *mutex = (pthread_mutex_t*)smalloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  worker_environment->mutex = mutex;

  worker_environment->valid           = true;
  worker_environment->connection_pool = connection_pool;
  worker_environment->configs         = configs;

  return worker_environment;
}

void destroy_worker_environment(worker_environment_t *worker_environment,
                                harp_list_t *ports) {
  if(worker_environment != NULL) {
    pthread_mutex_destroy(worker_environment->mutex);
    free(worker_environment->mutex);
    destroy_connection_pool(worker_environment->connection_pool, ports);
    harp_free_list(worker_environment->configs,
                   (harp_free_function_t*)&harp_free_config);
    free(worker_environment);
  }
}

void *worker(worker_environment_t *env) {

#define LOCK() do {                             \
    int rc = pthread_mutex_lock(env->mutex);    \
    if(rc != 0) {                               \
      logerror("worker:pthread_mutex_lock");    \
      pthread_exit(NULL);                       \
    }                                           \
  } while(0)

#define UNLOCK() do {                           \
    int rc = pthread_mutex_unlock(env->mutex);  \
    if(rc != 0) {                               \
      logerror("worker:pthread_mutex_unlock");  \
      pthread_exit(NULL);                       \
    }                                           \
  } while(0)

  while(!global_ready);

  nfds_t n;
  struct pollfd *pollfds;
  LOCK();
  create_pollfds(env->connection_pool, &pollfds, &n);
  UNLOCK();

  while(1) {
    LOCK();
    if(!env->valid) {
      UNLOCK();
      free(pollfds);
      pthread_exit(NULL);
    }
    aconnection_t *aconnection =
      get_next_connection(env->connection_pool, pollfds, n);
    UNLOCK();

    if(aconnection != NULL) {
      request_t *request = create_request(aconnection);
      if(request != NULL) {
        harp_config_t *config = find_config(env->configs, request->info,
                                             aconnection->addr_hash);
        resolution_strategy_t strategy = resolve_request(request, config);
        execute_fallback_strategy(request->aconnection->socket, strategy);
        harp_free_config(config);
        destroy_request(request); // Destroys the aconnection as well
      } else {
        destroy_aconnection(aconnection);
      }
    }
  }
}
