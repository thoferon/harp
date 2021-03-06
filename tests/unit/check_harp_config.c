#include <stdio.h>
#include <stdlib.h>
#include <check.h>

#include <harp.h>

/*
 * Functions to create the structures
 */

START_TEST (test_harp_make_hostnames_filter) {
  harp_filter_t *filter = harp_make_hostnames_filter((harp_list_t*)12345);

  ck_assert_int_eq(filter->type, HARP_FILTER_TYPE_HOSTNAMES);
  ck_assert(filter->hostnames == (harp_list_t*)12345);

  free(filter);
}
END_TEST

START_TEST (test_harp_make_ports_filter) {
  harp_filter_t *filter = harp_make_ports_filter((harp_list_t*)12345);

  ck_assert_int_eq(filter->type, HARP_FILTER_TYPE_PORTS);
  ck_assert(filter->ports == (harp_list_t*)12345);

  free(filter);
}
END_TEST

START_TEST (test_harp_make_static_path_resolver) {
  harp_resolver_t *resolver = harp_make_static_path_resolver((char *)12345);

  ck_assert_int_eq(resolver->type, HARP_RESOLVER_TYPE_STATIC_PATH);
  ck_assert(resolver->static_path == (char*)12345);

  free(resolver);
}
END_TEST

START_TEST (test_harp_make_server_resolver) {
  harp_resolver_t *resolver =
    harp_make_server_resolver(strdup("example.com"), 1337);

  ck_assert_int_eq(resolver->type, HARP_RESOLVER_TYPE_SERVER);
  ck_assert_str_eq(resolver->server->hostname, "example.com");
  ck_assert_int_eq(resolver->server->port, 1337);

  harp_free_resolver(resolver);
}
END_TEST

START_TEST (test_harp_make_empty_config) {
  harp_config_t *config = harp_make_empty_config();

  ck_assert(config->filters       == HARP_EMPTY_LIST);
  ck_assert(config->tags          == HARP_EMPTY_LIST);
  ck_assert(config->resolvers     == HARP_EMPTY_LIST);
  ck_assert(config->choice_groups == HARP_EMPTY_LIST);
  ck_assert(config->subconfigs    == HARP_EMPTY_LIST);

  harp_free_config(config);
}
END_TEST

START_TEST (test_harp_cons_filter) {
  harp_config_t *config = harp_make_empty_config();
  harp_cons_filter((harp_filter_t*)12345, config);

  ck_assert(config->filters->element == (harp_filter_t*)12345);
  ck_assert(config->tags             == HARP_EMPTY_LIST);
  ck_assert(config->resolvers        == HARP_EMPTY_LIST);
  ck_assert(config->choice_groups    == HARP_EMPTY_LIST);
  ck_assert(config->subconfigs       == HARP_EMPTY_LIST);

  free(config);
}
END_TEST

START_TEST (test_harp_cons_tag) {
  harp_config_t *config = harp_make_empty_config();
  harp_cons_tag((char*)12345, config);

  ck_assert(config->filters       == HARP_EMPTY_LIST);
  ck_assert(config->tags->element == (char*)12345);
  ck_assert(config->resolvers     == HARP_EMPTY_LIST);
  ck_assert(config->filters       == HARP_EMPTY_LIST);
  ck_assert(config->subconfigs    == HARP_EMPTY_LIST);

  free(config);
}
END_TEST

START_TEST (test_harp_cons_resolver) {
  harp_config_t *config = harp_make_empty_config();
  harp_cons_resolver((harp_resolver_t*)12345, config);

  ck_assert(config->filters            == HARP_EMPTY_LIST);
  ck_assert(config->tags               == HARP_EMPTY_LIST);
  ck_assert(config->resolvers->element == (harp_resolver_t*)12345);
  ck_assert(config->choice_groups      == HARP_EMPTY_LIST);
  ck_assert(config->subconfigs         == HARP_EMPTY_LIST);

  free(config);
}
END_TEST

START_TEST (test_harp_cons_choice_group) {
  harp_config_t *config = harp_make_empty_config();
  harp_cons_choice_group((harp_list_t*)12345, config);

  ck_assert(config->filters                == HARP_EMPTY_LIST);
  ck_assert(config->tags                   == HARP_EMPTY_LIST);
  ck_assert(config->resolvers              == HARP_EMPTY_LIST);
  ck_assert(config->choice_groups->element == (harp_list_t*)12345);
  ck_assert(config->subconfigs             == HARP_EMPTY_LIST);

  free(config);
}
END_TEST

START_TEST (test_harp_cons_subconfig) {
  harp_config_t *config    = harp_make_empty_config();
  harp_config_t *subconfig = harp_make_empty_config();
  harp_cons_subconfig(subconfig, config);

  ck_assert(config->filters             == HARP_EMPTY_LIST);
  ck_assert(config->tags                == HARP_EMPTY_LIST);
  ck_assert(config->resolvers           == HARP_EMPTY_LIST);
  ck_assert(config->choice_groups       == HARP_EMPTY_LIST);
  ck_assert(config->subconfigs->element == subconfig);

  harp_free_config(config);
}
END_TEST

START_TEST (test_harp_make_choice) {
  harp_choice_t *choice = harp_make_choice(42, (harp_config_t*)12345);

  ck_assert(choice->prob   == 42);
  ck_assert(choice->config == (harp_config_t*)12345);

  free(choice);
}
END_TEST

/*
 * Test for duplicate functions
 */

START_TEST (test_harp_duplicate_filter_with_hostnames) {
  harp_list_t *hostnames =
    harp_cons(strdup("example.com"), harp_singleton(strdup("example.net")));
  harp_filter_t *filter     = harp_make_hostnames_filter(hostnames);
  harp_filter_t *new_filter = harp_duplicate_filter(filter);
  harp_free_filter(filter);

  ck_assert_int_eq(new_filter->type, HARP_FILTER_TYPE_HOSTNAMES);
  ck_assert_str_eq((char*)new_filter->hostnames->element, "example.com");
  ck_assert_str_eq((char*)new_filter->hostnames->next->element, "example.net");

  harp_free_filter(new_filter);
}
END_TEST

START_TEST (test_harp_duplicate_filter_with_ports) {
  int *first_port  = (int*)malloc(sizeof(int));
  int *second_port = (int*)malloc(sizeof(int));
  *first_port      = 1;
  *second_port     = 2;

  harp_list_t *ports        = harp_cons(first_port,
                                        harp_singleton(second_port));
  harp_filter_t *filter     = harp_make_ports_filter(ports);
  harp_filter_t *new_filter = harp_duplicate_filter(filter);
  harp_free_filter(filter);

  ck_assert_int_eq(new_filter->type, HARP_FILTER_TYPE_PORTS);
  ck_assert_int_eq(*((int*)new_filter->ports->element), 1);
  ck_assert_int_eq(*((int*)new_filter->ports->next->element), 2);

  harp_free_filter(new_filter);
}
END_TEST

START_TEST (test_harp_duplicate_resolver_with_static_path) {
  harp_resolver_t *resolver =
    harp_make_static_path_resolver(strdup("/path"));
  harp_resolver_t *new_resolver = harp_duplicate_resolver(resolver);
  harp_free_resolver(resolver);

  ck_assert_int_eq(new_resolver->type, HARP_RESOLVER_TYPE_STATIC_PATH);
  ck_assert_str_eq(new_resolver->static_path, "/path");

  harp_free_resolver(new_resolver);
}
END_TEST

START_TEST (test_harp_duplicate_resolver_with_server) {
  harp_resolver_t *resolver =
    harp_make_server_resolver(strdup("example.com"), 1337);
  harp_resolver_t *new_resolver = harp_duplicate_resolver(resolver);
  harp_free_resolver(resolver);

  ck_assert_int_eq(new_resolver->type, HARP_RESOLVER_TYPE_SERVER);
  ck_assert_str_eq(new_resolver->server->hostname, "example.com");
  ck_assert_int_eq(new_resolver->server->port, 1337);

  harp_free_resolver(new_resolver);
}
END_TEST

START_TEST (test_harp_duplicate_choice) {
  harp_config_t *config     = harp_make_empty_config();
  harp_choice_t *choice     = harp_make_choice(42, config);
  harp_choice_t *new_choice = harp_duplicate_choice(choice);
  harp_free_choice(choice);

  ck_assert_int_eq(new_choice->prob, 42);
  ck_assert(new_choice->config != config);
  ck_assert(new_choice->config != NULL);

  harp_free_choice(new_choice);
}
END_TEST

START_TEST (test_harp_duplicate_choice_group) {
  harp_config_t *config1        = harp_make_empty_config();
  harp_choice_t *choice1        = harp_make_choice(42, config1);
  harp_config_t *config2        = harp_make_empty_config();
  harp_choice_t *choice2        = harp_make_choice(1337, config2);
  harp_list_t *choice_group     = harp_cons(choice1, harp_singleton(choice2));
  harp_list_t *new_choice_group = harp_duplicate_choice_group(choice_group);
  harp_free_choice_group(choice_group);

  harp_choice_t *new_choice1 = (harp_choice_t*)new_choice_group->element;
  harp_choice_t *new_choice2 = (harp_choice_t*)new_choice_group->next->element;

  ck_assert_int_eq(new_choice1->prob, 42);
  ck_assert(new_choice1->config != config1);
  ck_assert(new_choice1->config != config2);
  ck_assert(new_choice1->config != NULL);

  ck_assert_int_eq(new_choice2->prob, 1337);
  ck_assert(new_choice2->config != config1);
  ck_assert(new_choice2->config != config2);
  ck_assert(new_choice2->config != NULL);

  harp_free_choice_group(new_choice_group);
}
END_TEST

START_TEST (test_harp_duplicate_config) {
  harp_config_t *config     = harp_make_empty_config();
  harp_config_t *new_config = harp_duplicate_config(config);
  harp_free_config(config);

  ck_assert(new_config->filters       == HARP_EMPTY_LIST);
  ck_assert(new_config->tags          == HARP_EMPTY_LIST);
  ck_assert(new_config->resolvers     == HARP_EMPTY_LIST);
  ck_assert(new_config->choice_groups == HARP_EMPTY_LIST);
  ck_assert(new_config->subconfigs    == HARP_EMPTY_LIST);

  harp_free_config(new_config);
}
END_TEST

/*
 * Merging configurations
 */

START_TEST (test_harp_merge_configs) {
  harp_config_t *config1 = harp_make_empty_config();
  harp_config_t *config2 = harp_make_empty_config();

  harp_cons_tag(strdup("tag1"), config1);
  harp_cons_tag(strdup("tag2"), config2);

  harp_config_t *new_config = harp_merge_configs(config1, config2);
  harp_free_config(config1);
  harp_free_config(config2);

  ck_assert_str_eq(new_config->tags->element, "tag1");
  ck_assert_str_eq(new_config->tags->next->element, "tag2");

  harp_free_config(new_config);
}
END_TEST

/*
 * Memory-related test
 */

START_TEST (test_harp_free_config_on_full_structure) {
  harp_list_t *configs = harp_read_configs(FIXTURESDIR "/full.conf", NULL);

  if(configs == NULL) {
    char *msg;
    asprintf(&msg, "Can't read config: %s", harp_strerror(harp_errno));
    ck_abort_msg(msg);
  }

  // We can only ensure it doesn't fail
  harp_free_list(configs, (harp_free_function_t*)&harp_free_config);
}
END_TEST

START_TEST (test_harp_free_filter_with_NULL) {
  harp_free_filter(NULL);
}
END_TEST

START_TEST (test_harp_free_server_with_NULL) {
  harp_free_server(NULL);
}
END_TEST

START_TEST (test_harp_free_resolver_with_NULL) {
  harp_free_resolver(NULL);
}
END_TEST

START_TEST (test_harp_free_choice_with_NULL) {
  harp_free_choice(NULL);
}
END_TEST

START_TEST (test_harp_free_choice_group_with_NULL) {
  harp_free_choice_group(NULL);
}
END_TEST

START_TEST (test_harp_free_config_with_NULL) {
  harp_free_config(NULL);
}
END_TEST

/*
 * Getter functions
 */

START_TEST (test_harp_get_ports) {
  harp_list_t *configs = harp_read_configs(FIXTURESDIR "/full.conf", NULL);
  harp_list_t *ports   = harp_get_ports(configs);

  ck_assert_int_eq(*((int*)ports->element), 1);
  ck_assert(ports->next != NULL);
  ck_assert_int_eq(*((int*)ports->next->element), 2);
  ck_assert(ports->next->next != NULL);
  ck_assert_int_eq(*((int*)ports->next->next->element), 3);
  ck_assert(ports->next->next->next == NULL);

  harp_free_list(ports, NULL);
  harp_free_list(configs, (harp_free_function_t*)&harp_free_config);
}
END_TEST

/*
 * Miscellaneous tests
 */

START_TEST (test_include_directive) {
  harp_list_t *configs = harp_read_configs(FIXTURESDIR "/master.conf", NULL);
  if(configs == HARP_EMPTY_LIST) {
    ck_abort_msg(harp_strerror(harp_errno));
  }
  harp_config_t *config = configs->element;
  ck_assert(config->tags != HARP_EMPTY_LIST);
  ck_assert_str_eq(config->tags->element, "slave");
  harp_free_list(configs, (harp_free_function_t*)&harp_free_config);
}
END_TEST

/*
 * Suite
 */

Suite *harp_config_suite() {
  Suite *suite = suite_create("Config");

  TCase *tc_make = tcase_create("Make functions");
  tcase_add_test(tc_make, test_harp_make_hostnames_filter);
  tcase_add_test(tc_make, test_harp_make_ports_filter);
  tcase_add_test(tc_make, test_harp_make_static_path_resolver);
  tcase_add_test(tc_make, test_harp_make_server_resolver);
  tcase_add_test(tc_make, test_harp_make_empty_config);
  tcase_add_test(tc_make, test_harp_cons_filter);
  tcase_add_test(tc_make, test_harp_cons_tag);
  tcase_add_test(tc_make, test_harp_cons_resolver);
  tcase_add_test(tc_make, test_harp_cons_choice_group);
  tcase_add_test(tc_make, test_harp_cons_subconfig);
  tcase_add_test(tc_make, test_harp_make_choice);

  TCase *tc_duplicate = tcase_create("Duplicate functions");
  tcase_add_test(tc_duplicate, test_harp_duplicate_filter_with_hostnames);
  tcase_add_test(tc_duplicate, test_harp_duplicate_filter_with_ports);
  tcase_add_test(tc_duplicate, test_harp_duplicate_resolver_with_static_path);
  tcase_add_test(tc_duplicate, test_harp_duplicate_resolver_with_server);
  tcase_add_test(tc_duplicate, test_harp_duplicate_choice);
  tcase_add_test(tc_duplicate, test_harp_duplicate_choice_group);
  tcase_add_test(tc_duplicate, test_harp_duplicate_config);

  TCase *tc_merge = tcase_create("Merging configs");
  tcase_add_test(tc_merge, test_harp_merge_configs);

  TCase *tc_memory = tcase_create("Memory");
  tcase_add_test(tc_memory, test_harp_free_config_on_full_structure);
  tcase_add_test(tc_memory, test_harp_free_filter_with_NULL);
  tcase_add_test(tc_memory, test_harp_free_server_with_NULL);
  tcase_add_test(tc_memory, test_harp_free_resolver_with_NULL);
  tcase_add_test(tc_memory, test_harp_free_choice_with_NULL);
  tcase_add_test(tc_memory, test_harp_free_choice_group_with_NULL);
  tcase_add_test(tc_memory, test_harp_free_config_with_NULL);

  TCase *tc_getters = tcase_create("Getters");
  tcase_add_test(tc_getters, test_harp_get_ports);

  TCase *tc_misc = tcase_create("Miscellaneous");
  tcase_add_test(tc_misc, test_include_directive);

  suite_add_tcase(suite, tc_make);
  suite_add_tcase(suite, tc_duplicate);
  suite_add_tcase(suite, tc_merge);
  suite_add_tcase(suite, tc_memory);
  suite_add_tcase(suite, tc_getters);
  suite_add_tcase(suite, tc_misc);

  return suite;
}
