#include <stdlib.h>
#include <string.h>
#include <check.h>

#include <request.h>

/*
 * Functions to create structures
 */

START_TEST (test_make_request_info) {
  char *path     = strdup("/index.html");
  char *hostname = strdup("example.com");
  request_info_t *request_info = make_request_info(path, hostname, 80);

  ck_assert_str_eq(request_info->path, "/index.html");
  ck_assert_str_eq(request_info->hostname, "example.com");
  ck_assert_int_eq(request_info->port, 80);

  free_request_info(request_info);
}
END_TEST

START_TEST (test_make_request) {
  request_info_t *request_info = make_request_info(strdup("/"), strdup("example.com"), 80);
  request_t *request = make_request(request_info, (aconnection_t*)123,
                                    strdup("GET / HTTP/1.1"), 15);

  ck_assert_str_eq(request->info->path, "/");
  ck_assert_str_eq(request->info->hostname, "example.com");
  ck_assert_int_eq(request->info->port, 80);
  ck_assert_str_eq(request->buffer, "GET / HTTP/1.1");
  ck_assert_int_eq(request->buffer_size, 15);
  ck_assert(request->aconnection == (aconnection_t*)123);

  free_request(request);
}
END_TEST

/*
 * Suite
 */

Suite *request_suite() {
  Suite *suite = suite_create("Request info");

  TCase *tc_make = tcase_create("Make functions");
  tcase_add_test(tc_make, test_make_request_info);
  tcase_add_test(tc_make, test_make_request);

  suite_add_tcase(suite, tc_make);

  return suite;
}
