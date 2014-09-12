#include <check.h>
#include <stdlib.h>

#include <utils.h>

START_TEST (test_make_config_path_without_root_directory) {
  char *config_path = make_config_path(NULL, "/path");
  ck_assert_str_eq(config_path, "/path");
  free(config_path);
}
END_TEST

START_TEST (test_make_config_path_with_absolute_path) {
  char *config_path;

  config_path = make_config_path("/root/directory", "/path");
  ck_assert_str_eq(config_path, "/root/directory/path");
  free(config_path);

  config_path = make_config_path("/root/directory/", "/path");
  ck_assert_str_eq(config_path, "/root/directory/path");
  free(config_path);
}
END_TEST

START_TEST (test_make_config_path_with_relative_path) {
  // Because the working directory is set to the root directory after chroot()
  char *config_path;

  config_path = make_config_path("/root/directory", "path");
  ck_assert_str_eq(config_path, "/root/directory/path");
  free(config_path);

  config_path = make_config_path("/root/directory/", "path");
  ck_assert_str_eq(config_path, "/root/directory/path");
  free(config_path);
}
END_TEST

/*
 * Suite
 */

Suite *utils_suite() {
  Suite *suite = suite_create("Utils");

  TCase *tc_config_path = tcase_create("Config path");
  tcase_add_test(tc_config_path, test_make_config_path_without_root_directory);
  tcase_add_test(tc_config_path, test_make_config_path_with_absolute_path);
  tcase_add_test(tc_config_path, test_make_config_path_with_relative_path);

  suite_add_tcase(suite, tc_config_path);

  return suite;
}
