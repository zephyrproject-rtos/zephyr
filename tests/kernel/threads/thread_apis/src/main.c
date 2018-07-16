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
#include <kernel_structs.h>
#include <kernel.h>

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
extern void test_k_thread_foreach(void);

__kernel struct k_thread tdata;
#define STACK_SIZE (256 + CONFIG_TEST_EXTRA_STACKSIZE)
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);

/*local variables*/
static K_THREAD_STACK_DEFINE(tstack_custom, STACK_SIZE);
__kernel static struct k_thread tdata_custom;

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

static void customdata_entry(void *p1, void *p2, void *p3)
{
	u32_t data = 1;

	zassert_is_null(k_thread_custom_data_get(), NULL);
	while (1) {
		k_thread_custom_data_set((void *)data);
		/* relinguish cpu for a while */
		k_sleep(50);
		/** TESTPOINT: custom data comparison */
		zassert_equal(data, (u32_t)k_thread_custom_data_get(), NULL);
		data++;
	}
}

/**
 * @ingroup kernel_thread_tests
 * @brief test thread custom data get/set from coop thread
 *
 * @see k_thread_custom_data_get(), k_thread_custom_data_set()
 */
void test_customdata_get_set_coop(void)
{
	k_tid_t tid = k_thread_create(&tdata_custom, tstack_custom, STACK_SIZE,
				      customdata_entry, NULL, NULL, NULL,
				      K_PRIO_COOP(1), 0, 0);

	k_sleep(500);

	/* cleanup environment */
	k_thread_abort(tid);
}

/**
 * @ingroup kernel_thread_tests
 * @brief test thread custom data get/set from preempt thread
 * @see k_thread_custom_data_get(), k_thread_custom_data_set()
 */
void test_customdata_get_set_preempt(void)
{
	/** TESTPOINT: custom data of preempt thread */
	k_tid_t tid = k_thread_create(&tdata_custom, tstack_custom, STACK_SIZE,
				      customdata_entry, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(0), K_USER, 0);

	k_sleep(500);

	/* cleanup environment */
	k_thread_abort(tid);
}

#ifndef CONFIG_ARCH_HAS_USERSPACE
static void umode_entry(void *thread_id, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (!_is_thread_essential() &&
	    (k_current_get() == (k_tid_t)thread_id)) {
		ztest_test_pass();
	} else {
		zassert_unreachable("User thread is essential or thread"
				    " structure is corrupted\n");
	}
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test k_thread_user_mode_enter() to cover when userspace
 * is not supported/enabled
 * @see k_thread_user_mode_enter()
 */
void test_user_mode(void)
{
	_thread_essential_set();

	zassert_true(_is_thread_essential(), "Thread isn't set"
		     " as essential\n");

	k_thread_user_mode_enter((k_thread_entry_t)umode_entry,
				 k_current_get(), NULL, NULL);
}
#else
void test_user_mode(void)
{
	ztest_test_skip();
}
#endif

void test_main(void)
{
	k_thread_access_grant(k_current_get(), &tdata, tstack, NULL);
	k_thread_access_grant(k_current_get(), &tdata_custom, tstack_custom, NULL);
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
			 ztest_unit_test(test_systhreads_idle),
			 ztest_unit_test(test_customdata_get_set_coop),
			 ztest_user_unit_test(test_customdata_get_set_preempt),
			 ztest_unit_test(test_k_thread_foreach),
			 ztest_unit_test(test_user_mode)
			 );

	ztest_run_test_suite(threads_lifecycle);
}
