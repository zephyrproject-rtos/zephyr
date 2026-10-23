/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <string.h>
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

int task_wdt_add_ext(uint32_t reload_period, task_wdt_fault_callback_t callback, void *user_data)
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

int task_wdt_window_set(int channel_id, uint32_t min_period)
{
	ARG_UNUSED(channel_id);
	ARG_UNUSED(min_period);

	return 0;
}

int task_wdt_channel_thread_set(int channel_id, k_tid_t thread)
{
	ARG_UNUSED(channel_id);
	ARG_UNUSED(thread);

	return 0;
}

int task_wdt_channel_info_get(int channel_id, struct task_wdt_channel_info *info)
{
	ARG_UNUSED(channel_id);

	if (info == NULL) {
		return -EINVAL;
	}

	memset(info, 0, sizeof(*info));

	return 0;
}

int task_wdt_reset_info_get(struct task_wdt_reset_info *info)
{
	if (info == NULL) {
		return -EINVAL;
	}

	return -ENOENT;
}

int task_wdt_reset_info_clear(void)
{
	return 0;
}

int task_wdt_supervision_start(struct task_wdt_supervision *sup, int channel_id)
{
	ARG_UNUSED(sup);
	ARG_UNUSED(channel_id);

	return 0;
}

int task_wdt_supervision_stop(struct task_wdt_supervision *sup)
{
	ARG_UNUSED(sup);

	return 0;
}

int task_wdt_checkpoint(struct task_wdt_supervision *sup, uint32_t checkpoint)
{
	ARG_UNUSED(sup);
	ARG_UNUSED(checkpoint);

	return 0;
}

int task_wdt_channel_group_set(int channel_id, struct task_wdt_group *group)
{
	ARG_UNUSED(channel_id);
	ARG_UNUSED(group);

	return 0;
}

int task_wdt_group_action_get(const struct task_wdt_group *group)
{
	ARG_UNUSED(group);

	return TASK_WDT_ACTION_CALLBACK;
}

int task_wdt_group_reset(struct task_wdt_group *group)
{
	ARG_UNUSED(group);

	return 0;
}

void task_wdt_suspend(void)
{
}

void task_wdt_resume(void)
{
}
