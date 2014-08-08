#include <stdlib.h>

#include <memory.h>
#include <config.h>
#include <list.h>

/*
 * Operations on lists
 */

inline list_t *singleton(void *element) {
  return cons(element, EMPTY_LIST);
}

list_t *cons(void *element, list_t *rest) {
  list_t *list  = (list_t*)smalloc(sizeof(list));
  list->element = element;
  list->next    = rest;
  return list;
}

// Either the result or the two input lists should be freed but not both.
// This mutates the first list if not empty.
list_t *concat(list_t *first, list_t *second) {
  list_t *junction = last(first);
  if(junction == EMPTY_LIST) {
    return second;
  } else {
    junction->next = second;
    return first;
  }
}

list_t *last(list_t *list) {
  if(list == EMPTY_LIST) return EMPTY_LIST;
  list_t *current;
  for(current = list; current->next != EMPTY_LIST; current = current->next);
  return current;
}

// DO NOT free the source list
list_t *append(list_t *list, void *element) {
  if(list == EMPTY_LIST) {
    return singleton(element);
  } else {
    list_t *junction = last(list);
    junction->next = singleton(element);
    return list;
  }
}

list_t *duplicate(list_t *list, duplicate_function_t *duplicate_element) {
  if(list == EMPTY_LIST) {
    return EMPTY_LIST;
  } else {
    void *element = duplicate_element == NULL ? list->element : (*duplicate_element)(list->element);
    return cons(element, duplicate(list->next, duplicate_element));
  }
}

int length(list_t *list) {
  int len = 0;
  list_t *current;
  LISTFOREACH(current, list) { len++; }
  return len;
}

/*
 * Functions to free memory
 */

void free_list(list_t *list, free_function_t *free_element) {
  if(list != NULL) {
    if(list->next != NULL)
      free_list(list->next, free_element);
    if(free_element != NULL) (*free_element)(list->element);
    free(list);
  }
}

/*
 * Search functions
 */

void *find_element(list_t *list, predicate_function_t *predicate) {
  if(list == EMPTY_LIST) {
    return NULL;
  } else {
    list_t *current;
    LISTFOREACH(current, list) {
      if((*predicate)(current->element)) {
        return current->element;
      }
    }
    return NULL;
  }
}
