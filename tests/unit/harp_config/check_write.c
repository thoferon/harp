#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <harp.h>

START_TEST (test_harp_write_configs) {
  harp_list_t *configs = harp_read_configs(FIXTURESDIR "/full.conf", NULL);

  char *tpl = "/tmp/harp.test.XXXXXXXXXX";
  char *temp_path = (char*)malloc(strlen(tpl) + 1);
  strncpy(temp_path, tpl, strlen(tpl) + 1);
  if(mktemp(temp_path) == NULL) {
    perror("mktemp");
    ck_abort();
  }

  int rc = harp_write_configs(configs, temp_path);
  ck_assert_int_eq(rc, 0);

  FILE *f1 = fopen(FIXTURESDIR "/full.conf", "r");
  if(f1 == NULL) {
    perror("fopen");
    ck_abort();
  }

  FILE *f2 = fopen(temp_path, "r");
  if(f2 == NULL) {
    perror("fopen");
    ck_abort();
  }

  char buf1[300], buf2[300];

  size_t count1 = fread(&buf1, 1, 299, f1);
  size_t count2 = fread(&buf2, 1, 299, f2);

  buf1[299] = '\0';
  buf2[299] = '\0';

  ck_assert_int_ne(count1, 0);
  ck_assert_int_ne(count2, 0);

  ck_assert_str_eq(buf1, buf2);

  fclose(f1);
  fclose(f2);

  harp_free_list(configs, (harp_free_function_t*)&harp_free_config);
  free(temp_path);
}
END_TEST

/*
 * Suite
 */

Suite *harp_config_write_suite() {
  Suite *suite = suite_create("Write config");

  TCase *tc_all = tcase_create("Write");
  tcase_add_test(tc_all, test_harp_write_configs);

  suite_add_tcase(suite, tc_all);

  return suite;
}
