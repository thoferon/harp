#include <config.h>

#include <stdlib.h>
#include <stdio.h>

#include <memory.h>

inline void check_pointer(char *name, void *ptr) {
  if(ptr == NULL) {
    perror(name);
    exit(EXIT_FAILURE);
  }
}

void *smalloc(size_t size) {
  void *ptr = malloc(size);
  check_pointer("smalloc", ptr);
  return ptr;
}

void *scalloc(size_t nmemb, size_t size) {
  void *ptr = calloc(nmemb, size);
  check_pointer("scalloc", ptr);
  return ptr;
}

void *srealloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  check_pointer("srealloc", new_ptr);
  return new_ptr;
}
