/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(task_wdt_syswq_stall, CONFIG_LOG_DEFAULT_LEVEL);

static const uint32_t timeout_ms = CONFIG_TASK_WDT_SYSWQ_STALL_TIMEOUT_MS;
static const uint32_t feed_delay_ms = CONFIG_TASK_WDT_SYSWQ_STALL_TIMEOUT_MS / 2;

static int channel_id = -1;
static struct k_work_delayable dwork;

/** @brief Action taken when the system work queue is
 * unresponsive
 *
 * An unresponsive system work queue is indicative of a deadlock
 * involving the system work queue. The system work queue is
 * used by many subsystems, and a stuck system work queue likely
 * means the whole system is frozen. The default action is to
 * log an error message and trigger a kernel panic.
 *
 * This is a weak function that can be overriden by defining it
 * with the same name and type somewhere else.
 */
__weak void task_wdt_syswq_unresponsive(int _channel_id, void *user_data)
{
	ARG_UNUSED(_channel_id);
	ARG_UNUSED(user_data);

	LOG_ERR("Watch dog: System work queue unresponsive");
	k_oops();
}

static void feed_dog(struct k_work *work)
{
	ARG_UNUSED(work);

	task_wdt_feed(channel_id);
	k_work_schedule(&dwork, K_MSEC(feed_delay_ms));
}

static int init(void)
{
	int ret;

	ret = task_wdt_add(timeout_ms, task_wdt_syswq_unresponsive, NULL);

	if (ret < 0) {
		LOG_ERR("Failed to add task watchdog channel: %d", ret);
		return ret;
	}

	channel_id = ret;

	k_work_init_delayable(&dwork, feed_dog);
	k_work_schedule(&dwork, K_MSEC(feed_delay_ms));

	return 0;
}

SYS_INIT(init, APPLICATION, 0);
