#ifdef HAVE_CONFIG_H
#  include <config.h>
#else
#  define SYSLOG_LOGGING 1
#  define PACKAGE_STRING "harp (need config.h for version)"
#  define PACKAGE_BUGREPORT "see website"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>
#include <syslog.h>

#include <log.h>
#include <memory.h>
#include <options.h>

#define DEFAULT_CONFIG_PATH   (SYSCONFDIR "/harp.conf")
#define DEFAULT_THREAD_NUMBER 8

void usage(char *);
void show_version();

options_t *read_options(int argc, char **argv) {
  options_t *options     = (options_t*)scalloc(1, sizeof(struct options));
  options->background    = false;
  options->thread_number = DEFAULT_THREAD_NUMBER;

  struct option option_descriptors[] = {
    { "config-path",     required_argument, NULL, 'c' },
    { "thread-number",   required_argument, NULL, 'n' },
    { "background",      no_argument,       NULL, 'b' },
    { "user",            required_argument, NULL, 'u' },
    { "group",           required_argument, NULL, 'g' },
    { "chroot",          optional_argument, NULL, 1   },
#ifdef SYSLOG_LOGGING
    { "syslog-facility", required_argument, NULL, 2 },
#endif
    { "help",            no_argument,       NULL, 'h' },
    { "version",         no_argument,       NULL, 'V' },
    { NULL, 0, NULL, 0 }
  };

  char c;
  while((c = getopt_long(argc, argv, "c:n:bu:g:hV", option_descriptors, NULL))
        != -1) {
    switch(c) {
    case 'c': options->config_path   = strdup(optarg); break;
    case 'n': options->thread_number = atoi(optarg);   break;
    case 'b': options->background    = true;           break;
    case 'u': options->user          = strdup(optarg); break;
    case 'g': options->group         = strdup(optarg); break;
    case 1:
      if(optarg == NULL) {
        char buffer[PATH_MAX + 1];
        if(getcwd(buffer, PATH_MAX) == NULL) {
          logerror("read_options:getcwd");
          exit(EXIT_FAILURE);
        } else {
          options->chroot_directory = strdup(buffer);
        }
      } else {
        options->chroot_directory = strdup(optarg);
      }
      break;
#if SYSLOG_LOGGING
#define SETFACILITY(str, val) do {              \
        if(strcmp(optarg, (str)) == 0) {        \
          options->syslog_facility = (val);     \
        }                                       \
      } while(0)
    case 2:
      SETFACILITY("auth",     LOG_AUTH);
      SETFACILITY("authpriv", LOG_AUTHPRIV);
      SETFACILITY("cron",     LOG_CRON);
      SETFACILITY("daemon",   LOG_DAEMON);
      SETFACILITY("ftp",      LOG_FTP);
      SETFACILITY("lpr",      LOG_LPR);
      SETFACILITY("mail",     LOG_MAIL);
      SETFACILITY("news",     LOG_NEWS);
      SETFACILITY("syslog",   LOG_SYSLOG);
      SETFACILITY("user",     LOG_USER);
      SETFACILITY("uucp",     LOG_UUCP);
      SETFACILITY("local0",   LOG_LOCAL0);
      SETFACILITY("local1",   LOG_LOCAL1);
      SETFACILITY("local2",   LOG_LOCAL2);
      SETFACILITY("local3",   LOG_LOCAL3);
      SETFACILITY("local4",   LOG_LOCAL4);
      SETFACILITY("local5",   LOG_LOCAL5);
      SETFACILITY("local6",   LOG_LOCAL6);
      SETFACILITY("local7",   LOG_LOCAL7);
      break;
#endif

    case 'h': usage(argv[0]); exit(EXIT_SUCCESS);
    case 'V': show_version(); exit(EXIT_SUCCESS);
    default:  usage(argv[0]); exit(EXIT_FAILURE);
    }
  }

  if(options->config_path == NULL) {
    options->config_path = strdup(DEFAULT_CONFIG_PATH);
  }

  return options;
}

void free_options(options_t *options) {
  free(options->config_path);
  free(options->user);
  free(options->group);
  free(options->chroot_directory);
  free(options);
}

void usage(char *name) {
  fprintf(stderr, "Usage: %s [options]\n\n", name);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t-h --help\t\tShow this help text\n");
  fprintf(stderr, "\t-V --version\t\tShow the version\n");
  fprintf(stderr, "\t-c --config-path\tSpecify an alternative configuration file (default: %s)\n", DEFAULT_CONFIG_PATH);
  fprintf(stderr, "\t-n --thread-number\tNumber of worker threads (equivalent to the number of concurrent connections, default: %i)\n", DEFAULT_THREAD_NUMBER);
  fprintf(stderr, "\t-b --background\t\tRun the program in the background\n");
  fprintf(stderr, "\t-u --user\t\tUsername of the process\n");
  fprintf(stderr, "\t-g --group\t\tGroup of the process\n");
  fprintf(stderr, "\t   --chroot\t\tDirectory to chroot in\n");
#ifdef SYSLOG_LOGGING
  fprintf(stderr, "\t   --syslog-facility\tThe syslog facility to log to (e.g. user or local0)\n");
#endif
  fprintf(stderr, "\nBug-report: %s\n", PACKAGE_BUGREPORT);
}

void show_version() {
  printf("%s\n\n", PACKAGE_STRING);
  printf("Copyright (c) 2014 Thomas Feron\n");
  printf("License RBSD: BSD 3-clause <http://www.xfree86.org/3.3.6/COPYRIGHT2.html#5>\n");
  printf("Please send bug reports to %s.\n", PACKAGE_BUGREPORT);
}
