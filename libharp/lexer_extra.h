#ifndef LEXER_EXTRA_H
#define LEXER_EXTRA_H 1

#include <stdio.h>

#include <harp.h>

/* This file is needed to share definitions between harp_config.c and
 * config_lexer.l. Unfortunately, there does not seem to be a way to add code to
 * the generated config_lexer.h file.
 */

typedef struct extra {
  harp_list_t *files;
  char *root_directory;
} extra_t;

typedef struct extra_file {
  char *path;
  FILE *f;
} extra_file_t;

extra_file_t *make_extra_file(char *, FILE *);
void cons_extra_file(extra_file_t *, extra_t *);
void free_extra_file(extra_file_t *);

#endif
