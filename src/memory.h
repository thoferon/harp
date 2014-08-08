#ifndef MEMORY_H
#define MEMORY_H 1

#include <stdlib.h>

void *smalloc(size_t);
void *scalloc(size_t, size_t);
void *srealloc(void *, size_t);

#endif
