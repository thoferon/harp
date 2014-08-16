#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define PACKAGE_STRING "primp (need config.h for version)"
#define PACKAGE_BUGREPORT "see website"
#endif

#include <memory.h>
#include <options.h>

#define DEFAULT_CONFIG_PATH (SYSCONFDIR "/primp/primp.conf")

void usage(char *);
void show_version();

options_t *read_options(int argc, char **argv) {
  options_t *options = (options_t*)scalloc(1, sizeof(options));

  struct option option_descriptors[] = {
    { "help",        no_argument,       NULL, 'h' },
    { "version",     no_argument,       NULL, 'V' },
    { "config-path", required_argument, NULL, 'c' }
  };

  char c;
  while((c = getopt_long(argc, argv, "hVc:", option_descriptors, NULL)) != -1) {
    switch(c) {
    case 'h': usage(argv[0]); exit(EXIT_SUCCESS);
    case 'V': show_version(); exit(EXIT_SUCCESS);
    case 'c':
      options->config_path = strdup(optarg);
      break;
    default: usage(argv[0]); exit(EXIT_FAILURE);
    }
  }

  if(options->config_path == NULL) {
    options->config_path = strdup(DEFAULT_CONFIG_PATH);
  }

  return options;
}

void free_options(options_t *options) {
  free(options->config_path);
  free(options);
}

void usage(char *name) {
  fprintf(stderr, "Usage: %s [options]\n\n", name);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t-h --help\t\tShow this help text\n");
  fprintf(stderr, "\t-V --version\t\tShow the version\n");
  fprintf(stderr, "\t-c --config-path\tSpecify an alternative configuration file (default: %s)\n\n", DEFAULT_CONFIG_PATH);
  fprintf(stderr, "Bug-report: %s\n", PACKAGE_BUGREPORT);
}

void show_version() {
  printf("%s\n", PACKAGE_STRING);
  printf("Copyright (c) 2014 Thomas Feron\n");
  printf("License RBSD: BSD 3-clause <http://www.xfree86.org/3.3.6/COPYRIGHT2.html#5>\n");
  printf("Please send bug reports to %s.\n", PACKAGE_BUGREPORT);
}
