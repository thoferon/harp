#include <check.h>

#include <harp.h>
#include <request.h>

#include <harp_config/find.h>

START_TEST (test_find_config) {
  harp_list_t *configs = harp_read_configs(FIXTURESDIR "/full.conf");

  request_info_t *request_info1 =
    make_request_info(strdup("/"), strdup("blah"),  2);
  request_info_t *request_info2 =
    make_request_info(strdup("/"), strdup("blih"),  3);
  request_info_t *request_info3 =
    make_request_info(strdup("/"), strdup("bogus"), 3);

  harp_config_t *config1 = find_config(configs, request_info1, 0);
  harp_config_t *config2 = find_config(configs, request_info1, 0xFFFFFFFF);
  harp_config_t *config3 = find_config(configs, request_info2, 0);
  harp_config_t *config4 = find_config(configs, request_info3, 0);

  harp_resolver_t *resolver1 = (harp_resolver_t*)config1->resolvers->element;
  harp_resolver_t *resolver2 = (harp_resolver_t*)config2->resolvers->element;
  harp_resolver_t *resolver3 = (harp_resolver_t*)config3->resolvers->element;

  ck_assert_str_eq(resolver1->static_path, "/path1");
  ck_assert_str_eq(resolver2->static_path, "/path2");
  ck_assert_str_eq(resolver3->static_path, "/path3");
  ck_assert(config4 == NULL);

  free_request_info(request_info1);
  free_request_info(request_info2);
  free_request_info(request_info3);

  harp_free_config(config1);
  harp_free_config(config2);
  harp_free_config(config3);

  harp_free_list(configs, (harp_free_function_t*)&harp_free_config);
}
END_TEST

START_TEST (test_choose_config) {
  harp_list_t *configs  = harp_read_configs(FIXTURESDIR "/deep.conf");
  harp_config_t *config = configs->element;

  harp_config_t *config1 = choose_config(config, 0   << 24);
  harp_config_t *config2 = choose_config(config, 64  << 24);
  harp_config_t *config3 = choose_config(config, 127 << 24);
  harp_config_t *config4 = choose_config(config, 255 << 24);

  harp_resolver_t *resolver1 = (harp_resolver_t*)config1->resolvers->element;
  harp_resolver_t *resolver2 = (harp_resolver_t*)config2->resolvers->element;
  harp_resolver_t *resolver3 = (harp_resolver_t*)config3->resolvers->element;
  harp_resolver_t *resolver4 = (harp_resolver_t*)config4->resolvers->element;

  ck_assert(resolver1 != NULL);
  ck_assert_str_eq(resolver1->static_path, "/path11");
  ck_assert(resolver2 != NULL);
  ck_assert_str_eq(resolver2->static_path, "/path12");
  ck_assert(resolver3 != NULL);
  ck_assert_str_eq(resolver3->static_path, "/path22");
  ck_assert(resolver4 != NULL);
  ck_assert_str_eq(resolver4->static_path, "/path21");

  ck_assert_str_eq((char*)config1->tags->element, "tag1");
  ck_assert_str_eq((char*)config1->tags->next->element, "tag11");
  ck_assert(config1->tags->next->next == HARP_EMPTY_LIST);
  ck_assert_str_eq((char*)config2->tags->element, "tag1");
  ck_assert(config2->tags->next == HARP_EMPTY_LIST);
  ck_assert(config3->tags == HARP_EMPTY_LIST);
  ck_assert(config4->tags == HARP_EMPTY_LIST);

  harp_free_config(config1);
  harp_free_config(config2);
  harp_free_config(config3);
  harp_free_config(config4);

  harp_free_list(configs, (harp_free_function_t*)&harp_free_config);
}
END_TEST

/*
 * Suite
 */

Suite *harp_config_find_suite() {
  Suite *suite = suite_create("Find config");

  TCase *tc_all = tcase_create("All");
  tcase_add_test(tc_all, test_find_config);
  tcase_add_test(tc_all, test_choose_config);

  suite_add_tcase(suite, tc_all);

  return suite;
}
