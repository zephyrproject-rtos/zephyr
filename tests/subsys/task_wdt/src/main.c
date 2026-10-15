/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/ztest.h>

#define TASK_WDT_PERIOD_MS 100U
#define TASK_WDT_WAIT_MS   500U

K_SEM_DEFINE(callback_sem, 0, 1);
static atomic_t callback_count;
static atomic_t callback_channel;
static atomic_ptr_t callback_user_data;

static void task_wdt_callback(int channel_id, void *user_data)
{
	atomic_inc(&callback_count);
	atomic_set(&callback_channel, channel_id);
	atomic_ptr_set(&callback_user_data, user_data);
	k_sem_give(&callback_sem);
}

static void *task_wdt_setup(void)
{
	int ret = task_wdt_init(NULL);

	zassert_equal(ret, 0, "task_wdt_init() failed: %d", ret);

	return NULL;
}

static void task_wdt_before(void *arg)
{
	ARG_UNUSED(arg);

	k_sem_reset(&callback_sem);
	atomic_set(&callback_count, 0);
	atomic_set(&callback_channel, -1);
	atomic_ptr_set(&callback_user_data, NULL);
}

static void task_wdt_after(void *arg)
{
	ARG_UNUSED(arg);

	for (int channel_id = 0; channel_id < CONFIG_TASK_WDT_CHANNELS; channel_id++) {
		task_wdt_delete(channel_id);
	}
}

ZTEST(task_wdt, test_timeout)
{
	static int user_data;
	int channel_id;
	int ret;

	channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, task_wdt_callback, &user_data);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	ret = k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(ret, 0, "watchdog callback did not run");
	zassert_equal(atomic_get(&callback_count), 1);
	zassert_equal(atomic_get(&callback_channel), channel_id);
	zassert_equal(atomic_ptr_get(&callback_user_data), &user_data);
}

ZTEST(task_wdt, test_feed)
{
	int channel_id;
	int ret;

	channel_id = task_wdt_add(200U, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	k_sleep(K_MSEC(50));
	ret = task_wdt_feed(channel_id);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(100));
	zassert_equal(atomic_get(&callback_count), 0);

	ret = k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(ret, 0, "watchdog callback did not run after feed");
	zassert_equal(atomic_get(&callback_count), 1);
}

ZTEST(task_wdt, test_delete)
{
	int channel_id;
	int ret;

	channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	ret = task_wdt_delete(channel_id);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(atomic_get(&callback_count), 0);
}

ZTEST(task_wdt, test_suspend_resume)
{
	int channel_id;
	int ret;

	channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	task_wdt_suspend();
	k_sleep(K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(atomic_get(&callback_count), 0);

	task_wdt_resume();
	ret = k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(ret, 0, "watchdog callback did not run after resume");
	zassert_equal(atomic_get(&callback_count), 1);
	zassert_equal(atomic_get(&callback_channel), channel_id);
}

ZTEST_SUITE(task_wdt, NULL, task_wdt_setup, task_wdt_before, task_wdt_after, NULL);
