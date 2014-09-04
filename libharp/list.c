#include <stdlib.h>

#include <memory.h>
#include <config.h>
#include <harp.h>

/*
 * Operations on lists
 */

inline harp_list_t *harp_singleton(void *element) {
  return harp_cons(element, HARP_EMPTY_LIST);
}

harp_list_t *harp_cons(void *element, harp_list_t *rest) {
  harp_list_t *list  = (harp_list_t*)smalloc(sizeof(list));
  list->element = element;
  list->next    = rest;
  return list;
}

// Either the result or the two input lists should be freed but not both.
// This mutates the first list if not empty.
harp_list_t *harp_concat(harp_list_t *first, harp_list_t *second) {
  harp_list_t *junction = harp_last(first);
  if(junction == HARP_EMPTY_LIST) {
    return second;
  } else {
    junction->next = second;
    return first;
  }
}

harp_list_t *harp_last(harp_list_t *list) {
  if(list == HARP_EMPTY_LIST) return HARP_EMPTY_LIST;
  harp_list_t *current;
  for(current = list; current->next != HARP_EMPTY_LIST;
      current = current->next);
  return current;
}

// DO NOT free the source list
harp_list_t *harp_append(harp_list_t *list, void *element) {
  if(list == HARP_EMPTY_LIST) {
    return harp_singleton(element);
  } else {
    harp_list_t *junction = harp_last(list);
    junction->next = harp_singleton(element);
    return list;
  }
}

harp_list_t *harp_duplicate(harp_list_t *list,
                            harp_duplicate_function_t *duplicate_element) {
  if(list == HARP_EMPTY_LIST) {
    return HARP_EMPTY_LIST;
  } else {
    void *element = duplicate_element == NULL ?
      list->element : (*duplicate_element)(list->element);
    return harp_cons(element, harp_duplicate(list->next, duplicate_element));
  }
}

int harp_length(harp_list_t *list) {
  int len = 0;
  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, list) { len++; }
  return len;
}

/*
 * Functions to free memory
 */

void harp_free_list(harp_list_t *list, harp_free_function_t *free_element) {
  if(list != NULL) {
    if(list->next != NULL) {
      harp_free_list(list->next, free_element);
    }
    if(free_element != NULL) {
      (*free_element)(list->element);
    }
    free(list);
  }
}

/*
 * Search functions
 */

harp_list_t *harp_find_element(harp_list_t *list,
                               harp_predicate_function_t *predicate) {
  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, list) {
    if((*predicate)(current->element)) {
      break;
    }
  }
  return current;
}
