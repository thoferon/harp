#ifndef OPTIONS_H
#define OPTIONS_H 1

#include <stdbool.h>

typedef struct options {
  char *config_path;
  bool background;
  char *user;
  char *group;
  char *chroot_directory;
  int syslog_facility;
  int thread_number;
} options_t;

options_t *read_options(int argc, char **argv);

void free_options(options_t *);

#endif
