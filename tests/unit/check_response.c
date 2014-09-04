#include <check.h>

#include <response.h>

START_TEST (test_get_mime_type) {
  ck_assert_int_eq(get_mime_type("/path/file.html"), MIME_TYPE_HTML);
  ck_assert_int_eq(get_mime_type("path/file.html"),  MIME_TYPE_HTML);
  ck_assert_int_eq(get_mime_type("file.css"),        MIME_TYPE_CSS);
  ck_assert_int_eq(get_mime_type("css"),             MIME_TYPE_UNKNOWN);
  ck_assert_int_eq(get_mime_type("file.ext"),        MIME_TYPE_UNKNOWN);
}
END_TEST

/*
 * Suite
 */

Suite *response_suite() {
  Suite *suite = suite_create("HTTP Response");

  TCase *tc_mime_type = tcase_create("MIME type");
  tcase_add_test(tc_mime_type, test_get_mime_type);

  suite_add_tcase(suite, tc_mime_type);

  return suite;
}
