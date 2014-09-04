#include <config.h>

#ifdef USE_EXECINFO
#  include <execinfo.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#include <memory_tracking.h>

allocation_t *allocations = NULL;

/*
 * Functions to manage the list of allocations
 */

void add_allocation(void *ptr, size_t size) {
  void *(*libc_malloc)(size_t) = dlsym(RTLD_NEXT, "malloc");

  allocation_t *allocation = libc_malloc(sizeof(struct allocation));

#ifdef USE_EXECINFO
  void *function_pointers[3];
  int rc = backtrace(function_pointers, 3);
  if(rc == -1) {
    fprintf(stderr, "add_allocation: backtrace() failed\n");
    exit(EXIT_FAILURE);
  }
  allocation->function_pointer = function_pointers[2];
#else
  allocation->function_pointer = NULL;
#endif

  allocation->pointer          = ptr;
  allocation->size             = size;
  allocation->next             = allocations;

  allocations = allocation;
}

void move_allocation(void *ptr, void *new_ptr, size_t size) {
  if(ptr == NULL) { return; }

  allocation_t *current;
  for(current = allocations; current != NULL; current = current->next) {
    if(current->pointer == ptr) {
      current->pointer = new_ptr;
      current->size    = size;
      return;
    }
  }

  // It should have returned already
  fprintf(stderr,
          "move_allocation: couldn't find allocation with pointer = %p\n",
          ptr);
}

void remove_allocation(void *ptr) {
  if(ptr == NULL) { return; }

  void (*libc_free)(void*) = dlsym(RTLD_NEXT, "free");

  allocation_t *previous, *current;
  for(previous = NULL, current = allocations; current != NULL;
      previous = current, current = current->next) {

    if(current->pointer == ptr) {
      if(previous == NULL) {
        allocations = current->next;
      } else {
        previous->next = current->next;
      }
      libc_free(current);
      return;
    }
  }

  // It should have returned already
  fprintf(stderr,
          "remove_allocation: couldn't find allocation with pointer = %p\n",
          ptr);
}

/*
 * Overloaded allocation functions
 */

void *malloc(size_t size) {
  void *(*libc_malloc)(size_t) = dlsym(RTLD_NEXT, "malloc");
  void *ptr = libc_malloc(size);
  add_allocation(ptr, size);
  return ptr;
}

void *calloc(size_t nmemb, size_t size) {
  void *(*libc_calloc)(size_t,size_t) = dlsym(RTLD_NEXT, "calloc");
  void *ptr = libc_calloc(nmemb, size);
  add_allocation(ptr, nmemb * size);
  return ptr;
}

void *realloc(void *ptr, size_t size) {
  void *(*libc_realloc)(void*,size_t) = dlsym(RTLD_NEXT, "realloc");
  void *new_ptr = libc_realloc(ptr, size);
  move_allocation(ptr, new_ptr, size);
  return new_ptr;
}

void free(void *ptr) {
  void *(*libc_free)(void*) = dlsym(RTLD_NEXT, "free");
  libc_free(ptr);
  remove_allocation(ptr);
}

void cfree(void *ptr) {
  void *(*libc_cfree)(void*) = dlsym(RTLD_NEXT, "cfree");
  libc_cfree(ptr);
  remove_allocation(ptr);
}
