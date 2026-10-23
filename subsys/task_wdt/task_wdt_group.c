/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/task_wdt/task_wdt.h>

#include "task_wdt_internal.h"

static K_THREAD_STACK_DEFINE(recovery_stack, CONFIG_TASK_WDT_RECOVERY_STACK_SIZE);
static struct k_work_q recovery_queue;
/* one work item per channel, so the recoveries of different channels do not get lost */
static struct k_work recovery_work[CONFIG_TASK_WDT_CHANNELS];

static void recovery_handler(struct k_work *work)
{
	task_wdt_channel_recover(work - recovery_work);
}

void task_wdt_group_recovery_submit(int channel_id)
{
	k_work_submit_to_queue(&recovery_queue, &recovery_work[channel_id]);
}

enum task_wdt_action task_wdt_group_escalate(struct task_wdt_group *group, int64_t current_ticks)
{
	const struct task_wdt_escalation *esc = group->escalation;

	if (esc == NULL || esc->faults == 0U || group->level == TASK_WDT_ACTION_RESET_SYSTEM) {
		return group->level;
	}

	/* restart counting once the period elapsed */
	if (group->faults == 0U ||
	    (esc->period_ms != 0U &&
	     current_ticks - group->window_start_ticks >= k_ms_to_ticks_ceil64(esc->period_ms))) {
		group->faults = 0U;
		group->window_start_ticks = current_ticks;
	}

	group->faults++;
	if (group->faults >= esc->faults) {
		group->faults = 0U;
		group->level++;
	}

	return group->level;
}

int task_wdt_group_action_get(const struct task_wdt_group *group)
{
	k_spinlock_key_t key;
	int action;

	if (group == NULL) {
		return -EINVAL;
	}

	key = task_wdt_lock();
	action = group->level;
	task_wdt_unlock(key);

	return action;
}

int task_wdt_group_reset(struct task_wdt_group *group)
{
	k_spinlock_key_t key;

	if (group == NULL) {
		return -EINVAL;
	}

	key = task_wdt_lock();
	group->level = group->action;
	group->faults = 0U;
	task_wdt_group_channels_reset(group);
	task_wdt_unlock(key);

	return 0;
}

static int task_wdt_group_init(void)
{
	const struct k_work_queue_config cfg = {
		.name = "task_wdt_recovery",
	};

	for (size_t i = 0; i < ARRAY_SIZE(recovery_work); i++) {
		k_work_init(&recovery_work[i], recovery_handler);
	}

	k_work_queue_init(&recovery_queue);
	k_work_queue_start(&recovery_queue, recovery_stack, K_THREAD_STACK_SIZEOF(recovery_stack),
			   CONFIG_TASK_WDT_RECOVERY_PRIORITY, &cfg);

	return 0;
}

SYS_INIT(task_wdt_group_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
