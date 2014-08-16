#include <check.h>
#include <stdlib.h>
#include <stdbool.h>

#include <list.h>
#include <worker.h>

START_TEST (test_make_and_destroy_worker_environment) {
  worker_environment_t *env = make_worker_environment(123, 456);

  ck_assert(env->valid           == true);
  ck_assert(env->connection_pool == 123);
  ck_assert(env->configs         == 456);

  ck_assert_int_eq(pthread_mutex_lock(env->mutex), 0);

  // Clean it before destoy it
  env->connection_pool = EMPTY_LIST;
  env->configs         = EMPTY_LIST;

  destroy_worker_environment(env, false);
}
END_TEST

/*
 * Suite
 */

Suite *worker_suite() {
  Suite *suite = suite_create("Worker");

  TCase *tc_worker_environment = tcase_create("Environment");
  tcase_add_test(tc_worker_environment,
                 test_make_and_destroy_worker_environment);

  suite_add_tcase(suite, tc_worker_environment);

  return suite;
}
