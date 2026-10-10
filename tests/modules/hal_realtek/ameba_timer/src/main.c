/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <os_wrapper_timer.h>
#include <rtk_status.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#if K_HEAP_MEM_POOL_SIZE > 0

K_SEM_DEFINE(entered, 0, 1);
K_SEM_DEFINE(completed, 0, 8);
K_MUTEX_DEFINE(callback_mutex);
static atomic_t calls;
static bool self_delete;
static uint32_t expected_id;

static void callback(void *handle)
{
	zassert_false(k_is_in_isr(), "vendor callbacks must execute in thread context");
	zassert_equal(rtos_timer_get_id(handle), expected_id);
	atomic_inc(&calls);
	k_sem_give(&entered);
	/* In the regression case the caller holds this mutex while deleting
	 * the timer. Delete must enqueue/accept the command without joining us.
	 */
	zassert_ok(k_mutex_lock(&callback_mutex, K_FOREVER));
	if (self_delete) {
		zassert_equal(rtos_timer_delete(handle, RTOS_TIMER_MAX_DELAY), RTK_SUCCESS);
	}
	/* The timer remains alive until the current callback returns, including
	 * when another thread or this callback has accepted its deletion.
	 */
	zassert_equal(rtos_timer_get_id(handle), expected_id);
	zassert_ok(k_mutex_unlock(&callback_mutex));
	k_sem_give(&completed);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	atomic_clear(&calls);
	self_delete = false;
	expected_id = 0x1234;
	k_sem_reset(&entered);
	k_sem_reset(&completed);
}

static rtos_timer_t create_timer(uint32_t period, bool repeat, void (*fn)(void *p_context))
{
	rtos_timer_t timer = NULL;

	zassert_equal(rtos_timer_create(&timer, "test", expected_id, period, repeat, fn),
		      RTK_SUCCESS);
	zassert_not_null(timer);
	zassert_equal(rtos_timer_get_id(timer), expected_id);
	zassert_false(rtos_timer_is_timer_active(timer));
	return timer;
}

static void wait_callback(void)
{
	zassert_ok(k_sem_take(&completed, K_SECONDS(1)), "timer callback did not complete");
	/* completed is signalled inside the callback, before daemon retirement. */
	k_msleep(2);
}

ZTEST(ameba_timer, test_thread_context_and_mutex)
{
	rtos_timer_t timer = create_timer(1, false, callback);

	zassert_ok(k_mutex_lock(&callback_mutex, K_FOREVER));
	zassert_equal(rtos_timer_start(timer, 0), RTK_SUCCESS);
	zassert_ok(k_sem_take(&entered, K_SECONDS(1)));
	zassert_equal(k_sem_take(&completed, K_NO_WAIT), -EBUSY);
	zassert_false(rtos_timer_is_timer_active(timer));
	zassert_ok(k_mutex_unlock(&callback_mutex));
	wait_callback();
	zassert_equal(atomic_get(&calls), 1);
	zassert_equal(rtos_timer_delete(timer, 0), RTK_SUCCESS);
}

ZTEST(ameba_timer, test_delete_while_callback_waits_for_caller)
{
	rtos_timer_t timer = create_timer(1, true, callback);

	zassert_ok(k_mutex_lock(&callback_mutex, K_FOREVER));
	zassert_equal(rtos_timer_start(timer, 0), RTK_SUCCESS);
	zassert_ok(k_sem_take(&entered, K_SECONDS(1)));
	int64_t start = k_uptime_get();

	zassert_equal(rtos_timer_delete(timer, RTOS_TIMER_MAX_DELAY), RTK_SUCCESS);
	zassert_true(k_uptime_get() - start < 100, "delete waited for a blocked callback");
	zassert_equal(rtos_timer_start(timer, 0), RTK_FAIL);
	zassert_equal(rtos_timer_change_period(timer, 1, 0), RTK_FAIL);
	zassert_equal(rtos_timer_stop(timer, 0), RTK_FAIL);
	zassert_ok(k_mutex_unlock(&callback_mutex));
	wait_callback();
	k_msleep(20);
	zassert_equal(atomic_get(&calls), 1);
}

ZTEST(ameba_timer, test_periodic_self_delete)
{
	rtos_timer_t timer = create_timer(1, true, callback);

	self_delete = true;
	zassert_equal(rtos_timer_start(timer, 0), RTK_SUCCESS);
	wait_callback();
	k_msleep(20);
	zassert_equal(atomic_get(&calls), 1);
}

ZTEST(ameba_timer, test_stop_restart_and_change_period)
{
	rtos_timer_t timer = create_timer(30, false, callback);

	zassert_equal(rtos_timer_start(timer, 0), RTK_SUCCESS);
	zassert_true(rtos_timer_is_timer_active(timer));
	zassert_equal(rtos_timer_stop(timer, 0), RTK_SUCCESS);
	zassert_false(rtos_timer_is_timer_active(timer));
	k_msleep(50);
	zassert_equal(atomic_get(&calls), 0);
	zassert_equal(rtos_timer_change_period(timer, 1, 0), RTK_SUCCESS);
	wait_callback();
	zassert_equal(rtos_timer_start(timer, 0), RTK_SUCCESS);
	wait_callback();
	zassert_equal(atomic_get(&calls), 2);
	zassert_equal(rtos_timer_delete(timer, 0), RTK_SUCCESS);
}

static void rearm_callback(void *handle)
{
	zassert_false(k_is_in_isr());
	if (atomic_inc(&calls) == 0) {
		zassert_equal(rtos_timer_change_period(handle, 5, 0), RTK_SUCCESS);
	}
	k_sem_give(&completed);
}

ZTEST(ameba_timer, test_callback_rearms_one_shot)
{
	rtos_timer_t timer = create_timer(1, false, rearm_callback);

	zassert_equal(rtos_timer_start(timer, 0), RTK_SUCCESS);
	wait_callback();
	wait_callback();
	k_msleep(20);
	zassert_equal(atomic_get(&calls), 2);
	zassert_equal(rtos_timer_delete(timer, 0), RTK_SUCCESS);
}

static void stop_callback(void *handle)
{
	zassert_false(k_is_in_isr());
	if (atomic_inc(&calls) == 2) {
		zassert_equal(rtos_timer_stop(handle, 0), RTK_SUCCESS);
	}
	k_sem_give(&completed);
}

ZTEST(ameba_timer, test_periodic_callback_stop)
{
	rtos_timer_t timer = create_timer(5, true, stop_callback);

	zassert_equal(rtos_timer_start(timer, 0), RTK_SUCCESS);
	wait_callback();
	wait_callback();
	wait_callback();
	k_msleep(20);
	zassert_equal(atomic_get(&calls), 3);
	zassert_false(rtos_timer_is_timer_active(timer));
	zassert_equal(rtos_timer_delete(timer, 0), RTK_SUCCESS);
}

ZTEST(ameba_timer, test_zero_period_and_large_period)
{
	rtos_timer_t timer = create_timer(0, true, callback);

	zassert_equal(rtos_timer_start(timer, 0), RTK_SUCCESS);
	wait_callback();
	k_msleep(20);
	zassert_equal(atomic_get(&calls), 1);
	zassert_false(rtos_timer_is_timer_active(timer));
	zassert_equal(rtos_timer_change_period(timer, UINT32_MAX, 0), RTK_SUCCESS);
	zassert_true(rtos_timer_is_timer_active(timer));
	k_msleep(5);
	zassert_equal(atomic_get(&calls), 1);
	zassert_equal(rtos_timer_delete(timer, 0), RTK_SUCCESS);
}

ZTEST(ameba_timer, test_static_entry_points)
{
	rtos_timer_t timer = NULL;

	zassert_equal(rtos_timer_create_static(&timer, "test", expected_id, 1, false,
					       callback), RTK_SUCCESS);
	zassert_equal(rtos_timer_start(timer, 0), RTK_SUCCESS);
	wait_callback();
	zassert_equal(rtos_timer_delete_static(timer, 0), RTK_SUCCESS);
}

ZTEST(ameba_timer, test_invalid_arguments)
{
	rtos_timer_t timer;

	zassert_equal(rtos_timer_create(NULL, "test", 0, 1, false, callback), RTK_FAIL);
	zassert_equal(rtos_timer_create(&timer, "test", 0, 1, false, NULL), RTK_FAIL);
	zassert_equal(rtos_timer_start(NULL, 0), RTK_FAIL);
	zassert_equal(rtos_timer_stop(NULL, 0), RTK_FAIL);
	zassert_equal(rtos_timer_change_period(NULL, 1, 0), RTK_FAIL);
	zassert_equal(rtos_timer_delete(NULL, 0), RTK_FAIL);
	zassert_false(rtos_timer_is_timer_active(NULL));
	zassert_equal(rtos_timer_get_id(NULL), 0);
}

ZTEST_SUITE(ameba_timer, NULL, NULL, before, NULL, NULL);

#else

static void unexpected_callback(void *handle)
{
	ARG_UNUSED(handle);
	zassert_unreachable("creating a timer without a heap must fail");
}

ZTEST(ameba_timer_no_heap, test_create_fails_without_heap)
{
	rtos_timer_t timer = NULL;

	zassert_equal(rtos_timer_create(&timer, "test", 0, 1, false, unexpected_callback),
		      RTK_FAIL);
	zassert_is_null(timer);
	zassert_equal(rtos_timer_create_static(&timer, "test", 0, 1, false,
					       unexpected_callback), RTK_FAIL);
	zassert_is_null(timer);
}

ZTEST_SUITE(ameba_timer_no_heap, NULL, NULL, NULL, NULL, NULL);

#endif
