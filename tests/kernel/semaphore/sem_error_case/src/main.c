/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <ztest_error_hook.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define THREAD_TEST_PRIORITY 0

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static struct k_sem uninit_sem;

/**
 * @brief Fault injection test cases of semaphore
 * @defgroup semaphore_negative_tests Semaphore Fault Injection Tests
 * @ingroup kernel_semaphore_tests
 * @{
 * @}
 */

static void sem_thread_give_uninit(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);

	/* use sem without initialse */
	k_sem_give(&uninit_sem);

	ztest_test_fail();
}

/**
 * @brief Test k_sem_give() API
 *
 * @details Create a thread and use a uninitialized semaphore
 *
 * @ingroup semaphore_negative_tests
 *
 * @see k_sem_give()
 */
void test_sem_give_uninit(void)
{
	k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)sem_thread_give_uninit,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_sleep(K_MSEC(20));
	k_thread_join(&tdata, K_FOREVER);
}

static void thread_sem_give_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_sem_give(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_give() API
 *
 * @details Create a thread and set k_sem_give() input to NULL
 *
 * @ingroup semaphore_negative_tests
 *
 * @see k_sem_give()
 */
void test_sem_give_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_sem_give_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

static void thread_sem_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_sem_init(NULL, 0, 1);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_init() API
 *
 * @details Create a thread and set k_sem_init() input to NULL
 *
 * @ingroup semaphore_negative_tests
 *
 * @see k_sem_init()
 */
void test_sem_init_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_sem_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

static void thread_sem_take_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_sem_take(NULL, K_MSEC(1));

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_take() API
 *
 * @details Create a thread and set k_sem_take() input to NULL
 *
 * @ingroup semaphore_negative_tests
 *
 * @see k_sem_take()
 */
void test_sem_take_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_sem_take_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

static void thread_sem_reset_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_sem_reset(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_reset() API
 *
 * @details Create a thread and set k_sem_reset() input to NULL
 *
 * @ingroup semaphore_negative_tests
 *
 * @see k_sem_reset()
 */
void test_sem_reset_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_sem_reset_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

static void thread_sem_count_get_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_sem_count_get(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_count_get() API
 *
 * @details Create a thread and set k_sem_count_get() input to NULL
 *
 * @ingroup semaphore_negative_tests
 *
 * @see k_sem_count_get()
 */
void test_sem_count_get_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_sem_count_get_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/* ztest main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &tstack, &tdata,
				&uninit_sem);

	ztest_test_suite(test_semaphore_error,
			 ztest_user_unit_test(test_sem_give_uninit),
			 ztest_user_unit_test(test_sem_give_null),
			 ztest_user_unit_test(test_sem_init_null),
			 ztest_user_unit_test(test_sem_take_null),
			 ztest_user_unit_test(test_sem_reset_null),
			 ztest_user_unit_test(test_sem_count_get_null));
	ztest_run_test_suite(test_semaphore_error);
}
