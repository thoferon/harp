#include <check.h>

#include <harp.h>

#include <resolve.h>

START_TEST (test_resolve_request_with_null_config) {
  request_t *request             = make_request(NULL, NULL, NULL, 0);
  resolution_strategy_t strategy = resolve_request(request, NULL);

  ck_assert_int_eq(strategy, RESOLUTION_STRATEGY_400);

  free(request);
}
END_TEST

START_TEST (test_resolve_request_with_no_resolvers) {
  request_t *request    = make_request(NULL, NULL, NULL, 0);
  harp_config_t *config = harp_make_empty_config();

  resolution_strategy_t strategy = resolve_request(request, config);

  ck_assert_int_eq(strategy, RESOLUTION_STRATEGY_500);

  free(request);
  harp_free_config(config);
}
END_TEST

START_TEST (test_resolve_with_static_path_when_file_is_absent) {
  request_info_t *request_info =
    make_request_info(strdup("/absent"), strdup("example.com"), 80);
  request_t *request = make_request(request_info, NULL, strdup(""), 0);

  harp_resolver_t *resolver = harp_make_static_path_resolver(strdup("/tmp"));
  harp_config_t *config     = harp_make_empty_config();
  harp_cons_resolver(resolver, config);

  resolution_strategy_t strategy = resolve_request(request, config);

  ck_assert_int_eq(strategy, RESOLUTION_STRATEGY_404);

  harp_free_config(config);
  free_request(request);
}
END_TEST

/*
 * Suite
 */

Suite *resolve_suite() {
  Suite *suite = suite_create("Resolving requests");

  TCase *tc_resolve = tcase_create("Resolve functions");
  tcase_add_test(tc_resolve, test_resolve_request_with_null_config);
  tcase_add_test(tc_resolve, test_resolve_request_with_no_resolvers);
  tcase_add_test(tc_resolve, test_resolve_with_static_path_when_file_is_absent);

  suite_add_tcase(suite, tc_resolve);

  return suite;
}
