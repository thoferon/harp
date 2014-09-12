#include <stdio.h>

#include <smem.h>
#include <harp.h>

#include <lexer_extra.h>

extra_file_t *make_extra_file(char *path, FILE *f) {
  extra_file_t *extra_file = (extra_file_t*)smalloc(sizeof(struct extra_file));
  extra_file->path = path;
  extra_file->f    = f;
  return extra_file;
}

void cons_extra_file(extra_file_t *extra_file, extra_t *extra) {
  extra->files = harp_cons(extra_file, extra->files);
}

void free_extra_file(extra_file_t *extra_file) {
  if(extra_file->path != NULL) {
    free(extra_file->path);
  }
  free(extra_file);
}
