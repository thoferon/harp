#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <log.h>
#include <options.h>
#include <loader.h>

#include <start.h>

volatile bool global_ready = false;

int start(int argc, char **argv) {
  int rc;

  options_t *options = read_options(argc, argv);

  initialize_logger(options);

  loader_environment_t loader_environment;
  initialize_loader(&loader_environment, options);

  struct sigaction sigusr1_action;
  sigusr1_action.sa_handler = &sigusr1_handler;
  sigusr1_action.sa_mask    = 0;
  sigusr1_action.sa_flags   = 0;

  rc = sigaction(SIGUSR1, &sigusr1_action, NULL);
  if(rc != 0) {
    logerror("start:sigaction");
    return EXIT_FAILURE;
  }

  struct sigaction sigterm_action;
  sigterm_action.sa_handler = &sigterm_handler;
  sigterm_action.sa_mask    = 0;
  sigterm_action.sa_flags   = 0;

  rc = sigaction(SIGTERM, &sigterm_action, NULL);
  if(rc != 0) {
    logerror("start:sigaction");
    return EXIT_FAILURE;
  }

  reload_configuration(&loader_environment, options->chroot_directory);

  if(options->background) {
    pid_t pid = fork();
    if(pid == 0) {
      rc = setsid();
      if(rc == -1) {
        logerror("start:setsid");
        return EXIT_FAILURE;
      }
    } else if(pid == -1) {
      logerror("start:fork");
      return EXIT_FAILURE;
    } else {
      return EXIT_SUCCESS;
    }
  }

  if(options->chroot_directory) {
    rc = chroot(options->chroot_directory);
    if(rc == -1) {
      logerror("start:chroot");
      return EXIT_FAILURE;
    }

    rc = chdir("/");
    if(rc == -1) {
      logerror("start:chdir");
      return EXIT_FAILURE;
    }
  }

  if(options->group) {
    struct group *grp = getgrnam(options->group);
    if(grp == NULL) {
      logmsg(LOG_ERR, "start:getpwnam: Couldn't find group \"%s\"\n",
             options->group);
      return EXIT_FAILURE;
    }

    rc = setgid(grp->gr_gid);
    if(rc != 0) {
      logmsg(LOG_ERR, "start:setgid: Couldn't set the group ID\n");
      return EXIT_FAILURE;
    }
  }

  if(options->user) {
    struct passwd *pwd = getpwnam(options->user);
    if(pwd == NULL) {
      logmsg(LOG_ERR, "start:getpwnam: Couldn't find user \"%s\"\n",
             options->user);
      return EXIT_FAILURE;
    }

    rc = setuid(pwd->pw_uid);
    if(rc != 0) {
      logmsg(LOG_ERR, "start:setuid: Couldn't set the user ID\n");
      return EXIT_FAILURE;
    }
  }

  global_ready = true;

  do {
    if(check_reload_config_flag()) {
      reload_configuration(&loader_environment, NULL);
    }
    if(check_terminate_flag()) {
      break;
    }
    wait_for_flag_change();
  } while(1);

  terminate(&loader_environment);
  deinitialize_loader();
  free_options(options);

  return EXIT_SUCCESS;
}
