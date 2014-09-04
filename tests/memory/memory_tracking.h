#ifndef MEMORY_TRACKING_H
#define MEMORY_TRACKING_H 1

#include <stdlib.h>

typedef struct allocation {
  void *pointer;
  size_t size;
  void *function_pointer;
  struct allocation *next;
} allocation_t;

extern allocation_t *allocations;

void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
void cfree(void *);

#endif
