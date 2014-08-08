#include <check.h>
#include <stdlib.h>

#include <list.h>

/*
 * Operations on lists
 */

START_TEST (test_singleton) {
  int element  = 3;
  list_t *list = singleton(&element);

  ck_assert(list->element == &element);
  ck_assert(list->next == NULL);

  free(list);
}
END_TEST

START_TEST (test_cons) {
  int first_element  = 1;
  int second_element = 2;

  list_t *list     = singleton(&second_element);
  list_t *new_list = cons(&first_element, list);

  ck_assert(new_list->element == &first_element);
  ck_assert(new_list->next == list);

  free_list(new_list, NULL);
}
END_TEST

START_TEST (test_concat) {
  int first_element  = 1;
  int second_element = 2;
  int third_element  = 3;

  list_t *first_list  = cons(&first_element, singleton(&second_element));
  list_t *second_list = singleton(&third_element);

  list_t *new_list = concat(first_list, second_list);

  ck_assert(new_list->element == &first_element);
  ck_assert(new_list->next->element == &second_element);
  ck_assert(new_list->next->next->element == &third_element);
  ck_assert(new_list->next->next->next == NULL);

  free_list(new_list, NULL);
}
END_TEST

START_TEST(test_concat_when_first_is_empty) {
  int first_element = 1;

  list_t *first_list  = EMPTY_LIST;
  list_t *second_list = singleton(&first_element);

  list_t *new_list = concat(first_list, second_list);

  ck_assert(new_list == second_list);

  free_list(second_list, (free_function_t*)NULL);
}
END_TEST

START_TEST (test_last) {
  int first_element  = 1;
  int second_element = 2;

  list_t *list     = singleton(&second_element);
  list_t *new_list = cons(&first_element, list);

  ck_assert(last(new_list)->element == &second_element);

  free_list(new_list, NULL);
}
END_TEST

START_TEST (test_last_when_empty) {
  ck_assert(last(EMPTY_LIST) == EMPTY_LIST);
}
END_TEST

START_TEST (test_append) {
  int first_element  = 1;
  int second_element = 2;

  list_t *list = singleton(&first_element);
  list_t *new_list = append(list, &second_element);

  ck_assert(last(new_list)->element == &second_element);

  free_list(new_list, NULL);
}
END_TEST

START_TEST (test_append_to_empty_list) {
  int element = 1;
  list_t *list = append(EMPTY_LIST, &element);

  ck_assert(list->element == &element);
  ck_assert(list->next == EMPTY_LIST);

  free_list(list, NULL);
}
END_TEST

START_TEST (test_duplicate) {
  int first_element  = 1;
  int second_element = 2;

  list_t *list     = cons(&first_element, singleton(&second_element));
  list_t *new_list = duplicate(list, NULL);

  free_list(list, NULL);

  ck_assert(new_list->element == &first_element);
  ck_assert(new_list->next->element == &second_element);
  ck_assert(new_list->next->next == EMPTY_LIST);

  free_list(new_list, NULL);
}
END_TEST

int *duplicate_int(int *n) {
  int *m  = (int*)malloc(sizeof(int));
  *m = *n;
  return m;
}

START_TEST (test_duplicate_with_duplicate_function) {
  int first_element  = 1;
  int second_element = 2;

  list_t *list     = cons(&first_element, singleton(&second_element));
  list_t *new_list = duplicate(list, (duplicate_function_t*)&duplicate_int);

  ck_assert_int_eq(*((int*)new_list->element), first_element);
  ck_assert(new_list->element != &first_element);
  ck_assert_int_eq(*((int*)new_list->next->element), second_element);
  ck_assert(new_list->next->element != &second_element);
  ck_assert(new_list->next->next == EMPTY_LIST);

  free_list(list, NULL);
  free_list(new_list, (free_function_t*)&free);
}
END_TEST

START_TEST (test_length) {
  int first_element  = 1;
  int second_element = 2;

  list_t *list = cons(&first_element, singleton(&second_element));

  ck_assert_int_eq(length(list), 2);

  free_list(list, NULL);
}
END_TEST

START_TEST (test_length_on_empty_list) {
  ck_assert_int_eq(length(EMPTY_LIST), 0);
}
END_TEST

/*
 * Free memory from lists
 */

START_TEST (test_free_list_without_function) {
  int first_element  = 1;
  int second_element = 2;

  list_t *list = cons(&first_element, singleton(&second_element));

  // We can only ensure it doesn't fail.
  free_list(list, NULL);
}
END_TEST

START_TEST (test_free_list_with_function) {
  int *first_element_ptr  = (int*)malloc(sizeof(int));
  int *second_element_ptr = (int*)malloc(sizeof(int));

  *first_element_ptr  = 1;
  *second_element_ptr = 2;

  list_t *list = cons(first_element_ptr, singleton(second_element_ptr));

  // We can only ensure it doesn't fail.
  free_list(list, &free);
}
END_TEST

/*
 * Search functions
 */

bool is_two(int *ptr) {
  return *ptr == 2;
}

START_TEST (test_find_element) {
  int first_element  = 1;
  int second_element = 2;

  list_t *list = cons(&first_element, singleton(&second_element));

  ck_assert(find_element(list, (predicate_function_t*)&is_two) == &second_element);

  free_list(list, NULL);
}
END_TEST

START_TEST (test_find_element_in_empty_list) {
  ck_assert(find_element(NULL, (predicate_function_t*)&is_two) == NULL);
}
END_TEST

START_TEST (test_find_element_when_none_matches) {
  int first_element  = 1;

  list_t *list = singleton(&first_element);

  ck_assert(find_element(list, (predicate_function_t*)&is_two) == NULL);

  free_list(list, NULL);
}
END_TEST

/*
 * Suite
 */

Suite *list_suite() {
  Suite *suite = suite_create("List");

  TCase *tc_operations = tcase_create("Operations");
  tcase_add_test(tc_operations, test_singleton);
  tcase_add_test(tc_operations, test_cons);
  tcase_add_test(tc_operations, test_concat);
  tcase_add_test(tc_operations, test_concat_when_first_is_empty);
  tcase_add_test(tc_operations, test_last);
  tcase_add_test(tc_operations, test_last_when_empty);
  tcase_add_test(tc_operations, test_append);
  tcase_add_test(tc_operations, test_append_to_empty_list);
  tcase_add_test(tc_operations, test_duplicate);
  tcase_add_test(tc_operations, test_duplicate_with_duplicate_function);
  tcase_add_test(tc_operations, test_length);
  tcase_add_test(tc_operations, test_length_on_empty_list);

  TCase *tc_memory = tcase_create("Memory");
  tcase_add_test(tc_memory, test_free_list_without_function);
  tcase_add_test(tc_memory, test_free_list_with_function);

  TCase *tc_search = tcase_create("Search");
  tcase_add_test(tc_search, test_find_element);
  tcase_add_test(tc_search, test_find_element_in_empty_list);
  tcase_add_test(tc_search, test_find_element_when_none_matches);

  suite_add_tcase(suite, tc_operations);
  suite_add_tcase(suite, tc_memory);
  suite_add_tcase(suite, tc_search);

  return suite;
}
