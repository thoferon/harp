#ifndef WORKER_H
#define WORKER_H 1

#include <stdbool.h>
#include <pthread.h>

#include <harp.h>
#include <options.h>

typedef struct worker_environment {
  bool valid; // When false, the workers die
  pthread_mutex_t *mutex;
  harp_list_t *connection_pool;
  harp_list_t *configs;
} worker_environment_t;

worker_environment_t *make_worker_environment(harp_list_t *, harp_list_t *);
void destroy_worker_environment(worker_environment_t *, harp_list_t *);

void *worker(worker_environment_t *);

#endif
