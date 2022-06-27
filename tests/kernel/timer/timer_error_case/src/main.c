/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ztest.h>
#include <zephyr/types.h>
#include <zephyr/irq_offload.h>
#include <ztest_error_hook.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_TEST_PRIORITY 0
#define TEST_TIMEOUT -20
#define PERIOD 50
#define DURATION 100

static struct k_timer mytimer, sync_timer;
static struct k_thread tdata;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);

#ifdef CONFIG_USERSPACE
static void thread_timer_start_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_timer_start(NULL, K_MSEC(DURATION), K_NO_WAIT);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_timer_start() API
 *
 * @details Create a thread and set k_timer_start() input to NULL
 * and set a duration and period.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_start()
 */
void test_timer_start_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_timer_start_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_timer_start_null(void)
{
	/* Skip on platforms with no userspace support */
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_timer_stop_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_timer_stop(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_timer_stop() API
 *
 * @details Create a thread and set k_timer_stop() input to NULL
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_stop()
 */
void test_timer_stop_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_timer_stop_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_timer_stop_null(void)
{
	/* Skip on platforms with no userspace support */
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_timer_status_get_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_timer_status_get(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_timer_status_get() API
 *
 * @details Create a thread and set k_timer_status_get() input to NULL
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_status_get()
 */
void test_timer_status_get_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_timer_status_get_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_timer_status_get_null(void)
{
	/* Skip on platforms with no userspace support */
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_timer_status_sync_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_timer_status_sync(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_timer_status_sync() API
 *
 * @details Create a thread and set k_timer_status_sync() input to NULL
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_status_sync()
 */
void test_timer_status_sync_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_timer_status_sync_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_timer_status_sync_null(void)
{
	/* Skip on platforms with no userspace support */
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_timer_remaining_ticks_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_timer_remaining_ticks(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_timer_remaining_ticks() API
 *
 * @details Create a thread and set k_timer_remaining_ticks() input to NULL
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_remaining_ticks()
 */
void test_timer_remaining_ticks_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_timer_remaining_ticks_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_timer_remaining_ticks_null(void)
{
	/* Skip on platforms with no userspace support */
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_timer_expires_ticks_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_timer_expires_ticks(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_timer_expires_ticks() API
 *
 * @details Create a thread and set k_timer_expires_ticks() input to NULL
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_expires_ticks()
 */
void test_timer_expires_ticks_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_timer_expires_ticks_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_timer_expires_ticks_null(void)
{
	/* Skip on platforms with no userspace support */
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_timer_user_data_get_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_timer_user_data_get(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_timer_user_data_get() API
 *
 * @details Create a thread and set k_timer_user_data_get() input to NULL
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_user_data_get()
 */
void test_timer_user_data_get_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_timer_user_data_get_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_timer_user_data_get_null(void)
{
	/* Skip on platforms with no userspace support */
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_timer_user_data_set_null(void *p1, void *p2, void *p3)
{
	int user_data = 1;

	ztest_set_fault_valid(true);
	k_timer_user_data_set(NULL, &user_data);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_timer_user_data_set() API
 *
 * @details Create a thread and set k_timer_user_data_set() input to NULL
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_user_data_set()
 */
void test_timer_user_data_set_null(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_timer_user_data_set_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_timer_user_data_set_null(void)
{
	/* Skip on platforms with no userspace support */
	ztest_test_skip();
}
#endif

extern void z_add_timeout(struct _timeout *to, _timeout_func_t fn,
			  k_timeout_t timeout);
static void test_timer_handle(struct _timeout *t)
{
	/**do nothing here**/
}

void test_timer_add_timeout(void)
{
	struct _timeout tm;
	k_timeout_t timeout = K_FOREVER;

	z_add_timeout(&tm, test_timer_handle, timeout);
	ztest_test_pass();
}

extern uint64_t sys_clock_timeout_end_calc(k_timeout_t timeout);
extern void sys_clock_announce(int32_t ticks);
void test_timeout_end_calc(void)
{
	int ret;
	k_timeout_t timeout;

	timeout = K_MSEC(DURATION);
	ret = sys_clock_timeout_end_calc(timeout);
	zassert_true(ret, "---timeout end calc---");

	timeout.ticks = TEST_TIMEOUT;
	ret = sys_clock_timeout_end_calc(timeout);
	zassert_true(ret, "timeout end calc error");

	timeout = K_FOREVER;
	ret = sys_clock_timeout_end_calc(timeout);
	zassert_true(ret, "timeout end calc forever");
}

/**
 * @brief Tests for the Timer kernel object
 * @defgroup kernel_timer_tests Timer
 * @ingroup all_tests
 * @{
 * @}
 */

void test_main(void)
{
	k_thread_access_grant(k_current_get(), &tdata, &tstack, &mytimer, &sync_timer);
	ztest_test_suite(timer_api_error,
			 ztest_user_unit_test(test_timer_stop_null),
			 ztest_user_unit_test(test_timer_status_get_null),
			 ztest_user_unit_test(test_timer_status_sync_null),
			 ztest_user_unit_test(test_timer_remaining_ticks_null),
			 ztest_user_unit_test(test_timer_expires_ticks_null),
			 ztest_user_unit_test(test_timer_user_data_get_null),
			 ztest_user_unit_test(test_timer_user_data_set_null),
			 ztest_user_unit_test(test_timer_add_timeout),
			 ztest_unit_test(test_timeout_end_calc),
			 ztest_user_unit_test(test_timer_start_null));
	ztest_run_test_suite(timer_api_error);
}
