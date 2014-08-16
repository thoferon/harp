#ifndef LOADER_H
#define LOADER_H 1

#include <pthread.h>

#include <list.h>
#include <options.h>
#include <worker.h>

typedef struct loader_environment {
  options_t *options;
  pthread_t *threads;
  list_t *ports;
  worker_environment_t *worker_environment;
} loader_environment_t;

void initialize_loader(loader_environment_t *, options_t *);
void deinitialize_loader();

void wait_for_reload_config();

void sigusr1_handler(int);

void reload_configuration(loader_environment_t *);

#endif
