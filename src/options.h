#ifndef OPTIONS_H
#define OPTIONS_H 1

typedef struct options {
  char *config_path;
} options_t;

options_t *read_options(int argc, char **argv);

void free_options(options_t *);

#endif
