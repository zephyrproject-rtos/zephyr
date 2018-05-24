/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @brief Thread Tests
 * @defgroup kernel_thread_tests Threads
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>
extern void test_threads_spawn_params(void);
extern void test_threads_spawn_priority(void);
extern void test_threads_spawn_delay(void);
extern void test_threads_spawn_forever(void);
extern void test_thread_start(void);
extern void test_threads_suspend_resume_cooperative(void);
extern void test_threads_suspend_resume_preemptible(void);
extern void test_threads_abort_self(void);
extern void test_threads_abort_others(void);
extern void test_threads_abort_repeat(void);
extern void test_abort_handler(void);
extern void test_essential_thread_operation(void);
extern void test_threads_priority_set(void);
extern void test_delayed_thread_abort(void);

__kernel struct k_thread tdata;
#define STACK_SIZE (256 + CONFIG_TEST_EXTRA_STACKSIZE)
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);

static int main_prio;

/**
 * @ingroup kernel_thread_tests
 * @brief Verify main thread
 */
void test_systhreads_main(void)
{
	zassert_true(main_prio == CONFIG_MAIN_THREAD_PRIORITY, NULL);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Verify idle thread
 */
void test_systhreads_idle(void)
{
	k_sleep(100);
	/** TESTPOINT: check working thread priority should */
	zassert_true(k_thread_priority_get(k_current_get()) <
			K_IDLE_PRIO, NULL);
}

void test_main(void)
{
	k_thread_access_grant(k_current_get(), &tdata, tstack, NULL);
	main_prio = k_thread_priority_get(k_current_get());

	ztest_test_suite(threads_lifecycle,
			 ztest_user_unit_test(test_threads_spawn_params),
			 ztest_unit_test(test_threads_spawn_priority),
			 ztest_user_unit_test(test_threads_spawn_delay),
			 ztest_unit_test(test_threads_spawn_forever),
			 ztest_unit_test(test_thread_start),
			 ztest_unit_test(test_threads_suspend_resume_cooperative),
			 ztest_unit_test(test_threads_suspend_resume_preemptible),
			 ztest_unit_test(test_threads_priority_set),
			 ztest_user_unit_test(test_threads_abort_self),
			 ztest_user_unit_test(test_threads_abort_others),
			 ztest_unit_test(test_threads_abort_repeat),
			 ztest_unit_test(test_abort_handler),
			 ztest_unit_test(test_delayed_thread_abort),
			 ztest_unit_test(test_essential_thread_operation),
			 ztest_unit_test(test_systhreads_main),
			 ztest_unit_test(test_systhreads_idle)
			 );

	ztest_run_test_suite(threads_lifecycle);
}
