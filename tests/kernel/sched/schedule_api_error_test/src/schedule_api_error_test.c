/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include <kernel_internal.h>
#include <ztest_error_hook.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define THREAD_TEST_PRIORITY 0
static bool after_test;

static struct k_thread tdata;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);

static void thread_suspend_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_thread_suspend(NULL);

	/* should not go here */
	ztest_test_fail();
}

static void thread_resume_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_thread_resume(NULL);

	/* should not go here */
	ztest_test_fail();
}

static void thread_resume_unsuspend(void *p1, void *p2, void *p3)
{
	k_thread_resume(k_current_get());
	after_test = true;
}

static void thread_priority_get_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_thread_priority_get(NULL);

	/* should not go here */
	ztest_test_fail();
}

static void thread_priority_set_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_thread_priority_set(NULL, 0);

	/* should not go here */
	ztest_test_fail();
}

static void thread_priority_set_invalid1(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);

	/* set valid priority value outside the priority range will invoke fatal error */
	k_thread_priority_set(k_current_get(), K_LOWEST_APPLICATION_THREAD_PRIO + 1);

	/* should not go here */
	ztest_test_fail();
}

static void thread_priority_set_invalid2(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);

	/* set valid priority value to meet the usermode coverage branch */
	k_thread_priority_set(k_current_get(), THREAD_TEST_PRIORITY);
	/* test change thread priority in usermode cannot be upgraded */
	k_thread_priority_set(k_current_get(), THREAD_TEST_PRIORITY - 1);

	/* should not go here */
	ztest_test_fail();
}

static void thread_wakeup_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_wakeup(NULL);

	/* should not go here */
	ztest_test_fail();
}

/**
 * @brief Test k_thread_suspend() API
 *
 * @details Create a thread and set k_thread_suspend() input para to NULL
 * will trigger fatal error.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_suspend()
 */
void test_k_thread_suspend_init_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_suspend_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test k_thread_resume() API
 *
 * @details Create a thread and set k_thread_resume() para input to NULL
 * will trigger fatal error.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_resume()
 */
void test_k_thread_resume_init_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_resume_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test k_thread_resume() API
 *
 * @details resume a thread which is not suspended will directly return
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_resume()
 */
void test_k_thread_resume_unsuspend(void)
{
	after_test = false;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_resume_unsuspend,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			0, K_NO_WAIT);

	zassert_false(after_test, "child thread didn't return");

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test k_thread_priority_get() API
 *
 * @details Create a thread and set thread_k_thread_priority_get() para input to NULL
 *
 * @ingroup kernel_sched_tests
 *
 * @see thread_k_thread_priority_get()
 */
void test_k_thread_priority_get_init_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_priority_get_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test k_thread_priority_set() API
 *
 * @details Create a thread and set k_thread_priority_set() para input to NULL
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_priority_set()
 */
void test_k_thread_priority_set_init_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_priority_set_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test k_thread_priority_set() API
 *
 * @details Check input para range fail in userspace test.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_priority_set()
 */
void test_k_thread_priority_set_invalid1(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_priority_set_invalid1,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test k_thread_priority_set() API
 *
 * @details Check input para range fail in userspace test.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_priority_set()
 */
void test_k_thread_priority_set_invalid2(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_priority_set_invalid2,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}


/**
 * @brief Test k_wakeup() API
 *
 * @details Create a thread and set k_wakeup() para input to NULL
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_wakeup()
 */
void test_k_wakeup_init_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_wakeup_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/* ztest main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &tdata, &tstack);

	ztest_test_suite(test_schedule_api_error_test,
			 ztest_user_unit_test(test_k_thread_suspend_init_null),
			 ztest_user_unit_test(test_k_thread_resume_init_null),
			 ztest_unit_test(test_k_thread_resume_unsuspend),
			 ztest_user_unit_test(test_k_thread_priority_get_init_null),
			 ztest_user_unit_test(test_k_thread_priority_set_init_null),
			 ztest_user_unit_test(test_k_thread_priority_set_invalid1),
			 ztest_user_unit_test(test_k_thread_priority_set_invalid2),
			 ztest_user_unit_test(test_k_wakeup_init_null)
			 );
	ztest_run_test_suite(test_schedule_api_error_test);
}
