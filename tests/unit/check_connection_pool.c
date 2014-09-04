#include <stdlib.h>
#include <check.h>

#include <harp.h>

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

START_TEST (test_create_pollfds) {
  lconnection_t *lconnection1 = make_lconnection(1, 2);
  lconnection_t *lconnection2 = make_lconnection(3, 4);
  harp_list_t *lconnections   = harp_cons(lconnection1,
                                          harp_singleton(lconnection2));

  struct pollfd *pollfds;
  nfds_t n;
  create_pollfds(lconnections, &pollfds, &n);

  ck_assert_int_eq(n, 2);
  ck_assert_int_eq(pollfds[0].fd, 2);
  ck_assert_int_eq(pollfds[1].fd, 4);
  ck_assert_int_eq(pollfds[0].events, POLLIN);
  ck_assert_int_eq(pollfds[1].events, POLLIN);

  harp_free_list(lconnections, (harp_free_function_t*)free_lconnection);
}
END_TEST

/*
 * Suite
 */

Suite *connection_suite() {
  Suite *suite = suite_create("Connection pool");

  TCase *tc_all = tcase_create("All");
  tcase_add_test(tc_all, test_make_lconnection);
  tcase_add_test(tc_all, test_make_aconnection);
  tcase_add_test(tc_all, test_create_pollfds);

  suite_add_tcase(suite, tc_all);

  return suite;
}
