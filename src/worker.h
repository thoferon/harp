#ifndef WORKER_H
#define WORKER_H 1

#include <stdbool.h>
#include <pthread.h>

#include <list.h>

typedef struct worker_environment {
  bool valid; // When false, the workers die
  pthread_mutex_t *mutex;
  list_t *connection_pool;
  list_t *configs;
} worker_environment_t;

worker_environment_t *make_worker_environment(list_t *, list_t *);
void destroy_worker_environment(worker_environment_t *, list_t *);

void worker(worker_environment_t *);

#endif
