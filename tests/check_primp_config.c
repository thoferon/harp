#include <stdlib.h>
#include <check.h>

#include <list.h>
#include <primp_config.h>

/*
 * Functions to create the structures
 */

START_TEST (test_make_hostnames_filter) {
  filter_t *filter = make_hostnames_filter((list_t*)12345);

  ck_assert_int_eq(filter->type, FILTER_TYPE_HOSTNAMES);
  ck_assert(filter->hostnames == (list_t*)12345);

  free(filter);
}
END_TEST

START_TEST (test_make_ports_filter) {
  filter_t *filter = make_ports_filter((list_t*)12345);

  ck_assert_int_eq(filter->type, FILTER_TYPE_PORTS);
  ck_assert(filter->ports == (list_t*)12345);

  free(filter);
}
END_TEST

START_TEST (test_make_static_path_resolver) {
  resolver_t *resolver = make_static_path_resolver((char *)12345);

  ck_assert_int_eq(resolver->type, RESOLVER_TYPE_STATIC_PATH);
  ck_assert(resolver->static_path == (char*)12345);

  free(resolver);
}
END_TEST

START_TEST (test_make_server_resolver) {
  resolver_t *resolver = make_server_resolver(strdup("example.com"), 1337);

  ck_assert_int_eq(resolver->type, RESOLVER_TYPE_SERVER);
  ck_assert_str_eq(resolver->server->hostname, "example.com");
  ck_assert_int_eq(resolver->server->port, 1337);

  free_resolver(resolver);
}
END_TEST

START_TEST (test_make_empty_config) {
  config_t *config = make_empty_config();

  ck_assert(config->filters == NULL);
  ck_assert(config->resolvers == NULL);
  ck_assert(config->choice_groups == NULL);

  free_config(config);
}
END_TEST

START_TEST (test_cons_filter) {
  config_t *config = make_empty_config();
  cons_filter((filter_t*)12345, config);

  ck_assert(config->filters->element == (filter_t*)12345);
  ck_assert(config->resolvers == NULL);
  ck_assert(config->choice_groups == NULL);

  free(config);
}
END_TEST

START_TEST (test_cons_resolver) {
  config_t *config = make_empty_config();
  cons_resolver((resolver_t*)12345, config);

  ck_assert(config->filters == NULL);
  ck_assert(config->resolvers->element == (resolver_t*)12345);
  ck_assert(config->choice_groups == NULL);

  free(config);
}
END_TEST

START_TEST (test_cons_choice_group) {
  config_t *config = make_empty_config();
  cons_choice_group((list_t*)12345, config);

  ck_assert(config->filters == NULL);
  ck_assert(config->resolvers == NULL);
  ck_assert(config->choice_groups->element == (list_t*)12345);

  free(config);
}
END_TEST

START_TEST (test_make_choice) {
  choice_t *choice = make_choice(42, (config_t*)12345);

  ck_assert(choice->prob == 42);
  ck_assert(choice->config == (config_t*)12345);

  free(choice);
}
END_TEST

/*
 * Test for duplicate functions
 */

START_TEST (test_duplicate_filter_with_hostnames) {
  list_t *hostnames    = cons(strdup("example.com"), singleton(strdup("www.example.com")));
  filter_t *filter     = make_hostnames_filter(hostnames);
  filter_t *new_filter = duplicate_filter(filter);
  free_filter(filter);

  ck_assert_int_eq(new_filter->type, FILTER_TYPE_HOSTNAMES);
  ck_assert_str_eq((char*)new_filter->hostnames->element, "example.com");
  ck_assert_str_eq((char*)new_filter->hostnames->next->element, "www.example.com");

  free_filter(new_filter);
}
END_TEST

START_TEST (test_duplicate_filter_with_ports) {
  int *first_port  = (int*)malloc(sizeof(int));
  int *second_port = (int*)malloc(sizeof(int));
  *first_port      = 1;
  *second_port     = 2;

  list_t *ports        = cons(first_port, singleton(second_port));
  filter_t *filter     = make_ports_filter(ports);
  filter_t *new_filter = duplicate_filter(filter);
  free_filter(filter);

  ck_assert_int_eq(new_filter->type, FILTER_TYPE_PORTS);
  ck_assert_int_eq(*((int*)new_filter->ports->element), 1);
  ck_assert_int_eq(*((int*)new_filter->ports->next->element), 2);

  free_filter(new_filter);
}
END_TEST

START_TEST (test_duplicate_resolver_with_static_path) {
  resolver_t *resolver     = make_static_path_resolver(strdup("/path"));
  resolver_t *new_resolver = duplicate_resolver(resolver);
  free_resolver(resolver);

  ck_assert_int_eq(new_resolver->type, RESOLVER_TYPE_STATIC_PATH);
  ck_assert_str_eq(new_resolver->static_path, "/path");

  free_resolver(new_resolver);
}
END_TEST

START_TEST (test_duplicate_resolver_with_server) {
  resolver_t *resolver     = make_server_resolver(strdup("example.com"), 1337);
  resolver_t *new_resolver = duplicate_resolver(resolver);
  free_resolver(resolver);

  ck_assert_int_eq(new_resolver->type, RESOLVER_TYPE_SERVER);
  ck_assert_str_eq(new_resolver->server->hostname, "example.com");
  ck_assert_int_eq(new_resolver->server->port, 1337);

  free_resolver(new_resolver);
}
END_TEST

/*
 * Memory-related test
 */

START_TEST (test_free_config_on_full_structure) {
  list_t *configs = read_configs(FIXTURESDIR "/full.conf");

  // We can only ensure it doesn't fail
  free_list(configs, (free_function_t*)&free_config);
}
END_TEST

/*
 * Getter functions
 */

START_TEST (test_get_ports) {
  list_t *configs = read_configs(FIXTURESDIR "/full.conf");
  list_t *ports   = get_ports(configs);

  ck_assert_int_eq(*((int*)ports->element), 1);
  ck_assert(ports->next != NULL);
  ck_assert_int_eq(*((int*)ports->next->element), 2);
  ck_assert(ports->next->next != NULL);
  ck_assert_int_eq(*((int*)ports->next->next->element), 3);
  ck_assert(ports->next->next->next == NULL);

  free_list(ports, NULL);
  free_list(configs, (free_function_t*)&free_config);
}
END_TEST

/*
 * Suite
 */

Suite *primp_config_suite() {
  Suite *suite = suite_create("Config");

  TCase *tc_make = tcase_create("Make functions");
  tcase_add_test(tc_make, test_make_hostnames_filter);
  tcase_add_test(tc_make, test_make_ports_filter);
  tcase_add_test(tc_make, test_make_static_path_resolver);
  tcase_add_test(tc_make, test_make_server_resolver);
  tcase_add_test(tc_make, test_make_empty_config);
  tcase_add_test(tc_make, test_cons_filter);
  tcase_add_test(tc_make, test_cons_resolver);
  tcase_add_test(tc_make, test_cons_choice_group);
  tcase_add_test(tc_make, test_make_choice);

  TCase *tc_duplicate = tcase_create("Duplicate functions");
  tcase_add_test(tc_duplicate, test_duplicate_filter_with_hostnames);
  tcase_add_test(tc_duplicate, test_duplicate_filter_with_ports);
  tcase_add_test(tc_duplicate, test_duplicate_resolver_with_static_path);
  tcase_add_test(tc_duplicate, test_duplicate_resolver_with_server);

  TCase *tc_memory = tcase_create("Memory");
  tcase_add_test(tc_memory, test_free_config_on_full_structure);

  TCase *tc_getters = tcase_create("Getters");
  tcase_add_test(tc_getters, test_get_ports);

  suite_add_tcase(suite, tc_make);
  suite_add_tcase(suite, tc_duplicate);
  suite_add_tcase(suite, tc_memory);
  suite_add_tcase(suite, tc_getters);

  return suite;
}
