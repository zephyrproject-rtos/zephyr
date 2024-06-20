/* Copyright (c) 2023-2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/ztest_test.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test, LOG_LEVEL_DBG);

static struct k_sem stall_detected;

/* Intercept the panic. */
void task_wdt_syswq_unresponsive(int _channel_id, void *user_data)
{
	ARG_UNUSED(_channel_id);
	ARG_UNUSED(user_data);

	k_sem_give(&stall_detected);
}

static void forever_blocking_work_handler(struct k_work *work)
{
	while (true) {
		k_sleep(K_MSEC(100));
	}
}

ZTEST_SUITE(task_wdt_syswq_stall, NULL, NULL, NULL, NULL, NULL);
ZTEST(task_wdt_syswq_stall, test_detect_stall)
{
	int err;

	err = k_sem_init(&stall_detected, 0, 1);
	__ASSERT_NO_MSG(err == 0);

	/* Test that watchdog is fed normally. */
	err = k_sem_take(&stall_detected, K_MSEC(CONFIG_TASK_WDT_SYSWQ_STALL_TIMEOUT_MS * 2));
	zassert_equal(err, -EAGAIN, "False positive: Detected stall when not expected");

	/* Test that watchdog is triggered. */
	struct k_work work;
	k_work_init(&work, forever_blocking_work_handler);
	err = k_work_submit(&work);
	__ASSERT_NO_MSG(err == 1);

	err = k_sem_take(&stall_detected, K_MSEC(CONFIG_TASK_WDT_SYSWQ_STALL_TIMEOUT_MS * 2));
	zassert_equal(
		err, 0,
		"False negative: Stall not detected while test is blocking the system work queue");
}
