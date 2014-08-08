#ifndef LIST_H
#define LIST_H 1

#include <stdbool.h>

#define EMPTY_LIST NULL

typedef void free_function_t(void *);
typedef void *duplicate_function_t(void *);
typedef bool predicate_function_t(void *);

typedef struct list {
  void *element;
  struct list *next;
} list_t;

list_t *singleton(void *);
list_t *cons(void *, list_t *);
list_t *concat(list_t *, list_t *);
list_t *last(list_t *);
list_t *append(list_t *, void *);
list_t *duplicate(list_t *, duplicate_function_t *);
int length(list_t *);

void free_list(list_t *, free_function_t *);

void *find_element(list_t *, predicate_function_t *);

#define LISTFOREACH(varname, list) for(varname = (list); varname != EMPTY_LIST; varname = varname->next)

#endif
