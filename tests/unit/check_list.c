#include <check.h>
#include <stdlib.h>

#include <harp.h>

/*
 * Operations on lists
 */

START_TEST (test_harp_singleton) {
  int element  = 3;
  harp_list_t *list = harp_singleton(&element);

  ck_assert(list->element == &element);
  ck_assert(list->next == NULL);

  free(list);
}
END_TEST

START_TEST (test_harp_cons) {
  int first_element  = 1;
  int second_element = 2;

  harp_list_t *list     = harp_singleton(&second_element);
  harp_list_t *new_list = harp_cons(&first_element, list);

  ck_assert(new_list->element == &first_element);
  ck_assert(new_list->next    == list);

  harp_free_list(new_list, NULL);
}
END_TEST

START_TEST (test_harp_concat) {
  int first_element  = 1;
  int second_element = 2;
  int third_element  = 3;

  harp_list_t *first_list = harp_cons(&first_element,
                                      harp_singleton(&second_element));
  harp_list_t *second_list = harp_singleton(&third_element);

  harp_list_t *new_list = harp_concat(first_list, second_list);

  ck_assert(new_list->element             == &first_element);
  ck_assert(new_list->next->element       == &second_element);
  ck_assert(new_list->next->next->element == &third_element);
  ck_assert(new_list->next->next->next    == NULL);

  harp_free_list(new_list, NULL);
}
END_TEST

START_TEST(test_harp_concat_when_first_is_empty) {
  int first_element = 1;

  harp_list_t *first_list  = HARP_EMPTY_LIST;
  harp_list_t *second_list = harp_singleton(&first_element);

  harp_list_t *new_list = harp_concat(first_list, second_list);

  ck_assert(new_list == second_list);

  harp_free_list(second_list, (harp_free_function_t*)NULL);
}
END_TEST

START_TEST (test_harp_last) {
  int first_element  = 1;
  int second_element = 2;

  harp_list_t *list     = harp_singleton(&second_element);
  harp_list_t *new_list = harp_cons(&first_element, list);

  ck_assert(harp_last(new_list)->element == &second_element);

  harp_free_list(new_list, NULL);
}
END_TEST

START_TEST (test_harp_last_when_empty) {
  ck_assert(harp_last(HARP_EMPTY_LIST) == HARP_EMPTY_LIST);
}
END_TEST

START_TEST (test_harp_append) {
  int first_element  = 1;
  int second_element = 2;

  harp_list_t *list     = harp_singleton(&first_element);
  harp_list_t *new_list = harp_append(list, &second_element);

  ck_assert(harp_last(new_list)->element == &second_element);

  harp_free_list(new_list, NULL);
}
END_TEST

START_TEST (test_harp_append_to_empty_list) {
  int element = 1;
  harp_list_t *list = harp_append(HARP_EMPTY_LIST, &element);

  ck_assert(list->element == &element);
  ck_assert(list->next == HARP_EMPTY_LIST);

  harp_free_list(list, NULL);
}
END_TEST

START_TEST (test_harp_duplicate) {
  int first_element  = 1;
  int second_element = 2;

  harp_list_t *list = harp_cons(&first_element,
                                harp_singleton(&second_element));
  harp_list_t *new_list = harp_duplicate(list, NULL);

  harp_free_list(list, NULL);

  ck_assert(new_list->element       == &first_element);
  ck_assert(new_list->next->element == &second_element);
  ck_assert(new_list->next->next    == HARP_EMPTY_LIST);

  harp_free_list(new_list, NULL);
}
END_TEST

int *duplicate_int(int *n) {
  int *m  = (int*)malloc(sizeof(int));
  *m = *n;
  return m;
}

START_TEST (test_harp_duplicate_with_duplicate_function) {
  int first_element  = 1;
  int second_element = 2;

  harp_list_t *list =
    harp_cons(&first_element, harp_singleton(&second_element));
  harp_list_t *new_list =
    harp_duplicate(list, (harp_duplicate_function_t*)&duplicate_int);

  ck_assert_int_eq(*((int*)new_list->element), first_element);
  ck_assert(new_list->element != &first_element);
  ck_assert_int_eq(*((int*)new_list->next->element), second_element);
  ck_assert(new_list->next->element != &second_element);
  ck_assert(new_list->next->next == HARP_EMPTY_LIST);

  harp_free_list(list, NULL);
  harp_free_list(new_list, (harp_free_function_t*)&free);
}
END_TEST

START_TEST (test_harp_length) {
  int first_element  = 1;
  int second_element = 2;

  harp_list_t *list = harp_cons(&first_element,
                                harp_singleton(&second_element));

  ck_assert_int_eq(harp_length(list), 2);

  harp_free_list(list, NULL);
}
END_TEST

START_TEST (test_harp_length_on_empty_list) {
  ck_assert_int_eq(harp_length(HARP_EMPTY_LIST), 0);
}
END_TEST

/*
 * Free memory from lists
 */

START_TEST (test_harp_free_list_without_function) {
  int first_element  = 1;
  int second_element = 2;

  harp_list_t *list = harp_cons(&first_element,
                                harp_singleton(&second_element));

  // We can only ensure it doesn't fail.
  harp_free_list(list, NULL);
}
END_TEST

START_TEST (test_harp_free_list_with_function) {
  int *first_element_ptr  = (int*)malloc(sizeof(int));
  int *second_element_ptr = (int*)malloc(sizeof(int));

  *first_element_ptr  = 1;
  *second_element_ptr = 2;

  harp_list_t *list = harp_cons(first_element_ptr,
                                harp_singleton(second_element_ptr));

  // We can only ensure it doesn't fail.
  harp_free_list(list, &free);
}
END_TEST

/*
 * Search functions
 */

bool is_two(int *ptr) {
  return *ptr == 2;
}

START_TEST (test_harp_find_element) {
  int first_element  = 1;
  int second_element = 2;

  harp_list_t *list = harp_cons(&first_element,
                                harp_singleton(&second_element));

  harp_list_t *found_node =
    harp_find_element(list, (harp_predicate_function_t*)&is_two);
  ck_assert(found_node->element == &second_element);

  harp_free_list(list, NULL);
}
END_TEST

START_TEST (test_harp_find_element_in_empty_list) {
  harp_list_t *found_node =
    harp_find_element(NULL, (harp_predicate_function_t*)&is_two);
  ck_assert(found_node == HARP_EMPTY_LIST);
}
END_TEST

START_TEST (test_harp_find_element_when_none_matches) {
  int first_element = 1;

  harp_list_t *list = harp_singleton(&first_element);

  void *found_element =
    harp_find_element(list, (harp_predicate_function_t*)&is_two);
  ck_assert(found_element == HARP_EMPTY_LIST);

  harp_free_list(list, NULL);
}
END_TEST

/*
 * Suite
 */

Suite *list_suite() {
  Suite *suite = suite_create("List");

  TCase *tc_operations = tcase_create("Operations");
  tcase_add_test(tc_operations, test_harp_singleton);
  tcase_add_test(tc_operations, test_harp_cons);
  tcase_add_test(tc_operations, test_harp_concat);
  tcase_add_test(tc_operations, test_harp_concat_when_first_is_empty);
  tcase_add_test(tc_operations, test_harp_last);
  tcase_add_test(tc_operations, test_harp_last_when_empty);
  tcase_add_test(tc_operations, test_harp_append);
  tcase_add_test(tc_operations, test_harp_append_to_empty_list);
  tcase_add_test(tc_operations, test_harp_duplicate);
  tcase_add_test(tc_operations, test_harp_duplicate_with_duplicate_function);
  tcase_add_test(tc_operations, test_harp_length);
  tcase_add_test(tc_operations, test_harp_length_on_empty_list);

  TCase *tc_memory = tcase_create("Memory");
  tcase_add_test(tc_memory, test_harp_free_list_without_function);
  tcase_add_test(tc_memory, test_harp_free_list_with_function);

  TCase *tc_search = tcase_create("Search");
  tcase_add_test(tc_search, test_harp_find_element);
  tcase_add_test(tc_search, test_harp_find_element_in_empty_list);
  tcase_add_test(tc_search, test_harp_find_element_when_none_matches);

  suite_add_tcase(suite, tc_operations);
  suite_add_tcase(suite, tc_memory);
  suite_add_tcase(suite, tc_search);

  return suite;
}
