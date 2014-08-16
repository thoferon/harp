#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <log.h>
#include <options.h>
#include <loader.h>

int main(int argc, char **argv) {
  int rc;

  options_t *options = read_options(argc, argv);

  loader_environment_t loader_environment;
  initialize_loader(&loader_environment, options);

  struct sigaction sigusr1_action;
  sigusr1_action.sa_handler = &sigusr1_handler;
  sigusr1_action.sa_mask    = 0;
  sigusr1_action.sa_flags   = 0;

  rc = sigaction(SIGUSR1, &sigusr1_action, NULL);
  if(rc != 0) {
    logerror("main:sigaction");
    return EXIT_FAILURE;
  }

  do {
    reload_configuration(&loader_environment);
    wait_for_reload_config();
  } while(1);

  // Never going to be called but, later on, it might
  deinitialize_loader();
}
