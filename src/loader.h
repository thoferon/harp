#ifndef LOADER_H
#define LOADER_H 1

#include <pthread.h>

#include <harp.h>
#include <options.h>
#include <worker.h>

typedef struct loader_environment {
  options_t *options;
  pthread_t *threads;
  harp_list_t *ports;
  worker_environment_t *worker_environment;
} loader_environment_t;

void initialize_loader(loader_environment_t *, options_t *);
void deinitialize_loader();

void wait_for_flag_change();
bool check_reload_config_flag();
bool check_terminate_flag();

void sighup_handler(int);
void sigterm_handler(int);

void reload_configuration(loader_environment_t *, char *);
void terminate(loader_environment_t *);

#endif
