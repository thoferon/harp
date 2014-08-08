#include <stdlib.h>
#include <check.h>

#include <connection_pool.h>

/*
 * Functions to create structures
 */

START_TEST (test_make_lconnection) {
  lconnection_t *lconnection = make_lconnection(1, 2);

  ck_assert_int_eq(lconnection->port, 1);
  ck_assert_int_eq(lconnection->socket, 2);

  free_lconnection(lconnection);
}
END_TEST

START_TEST (test_make_aconnection) {
  aconnection_t *aconnection = make_aconnection(0, 1, 2);

  ck_assert_int_eq(aconnection->addr_hash, 0);
  ck_assert_int_eq(aconnection->port, 1);
  ck_assert_int_eq(aconnection->socket, 2);

  free_aconnection(aconnection);
}
END_TEST

/*
 * Suite
 */

Suite *connection_suite() {
  Suite *suite = suite_create("Connection pool");

  TCase *tc_make = tcase_create("Make functions");
  tcase_add_test(tc_make, test_make_lconnection);
  tcase_add_test(tc_make, test_make_aconnection);

  suite_add_tcase(suite, tc_make);

  return suite;
}
