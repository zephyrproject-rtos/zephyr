/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(task_wdt);

int task_wdt_init(const struct device *hw_wdt)
{
	ARG_UNUSED(hw_wdt);

	LOG_WRN("Dummy Task Watchdog implementation enabled.");

	return 0;
}

int task_wdt_add(uint32_t reload_period, task_wdt_callback_t callback,
		 void *user_data)
{
	ARG_UNUSED(reload_period);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return 0;
}

int task_wdt_delete(int channel_id)
{
	ARG_UNUSED(channel_id);

	return 0;
}

int task_wdt_feed(int channel_id)
{
	ARG_UNUSED(channel_id);

	return 0;
}

void task_wdt_suspend(void)
{
}

void task_wdt_resume(void)
{
}
