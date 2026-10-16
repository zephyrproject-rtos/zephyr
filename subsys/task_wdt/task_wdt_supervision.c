/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/task_wdt/task_wdt.h>

#include "task_wdt_internal.h"

static bool deadlines_valid(const struct task_wdt_supervision *sup)
{
	for (size_t i = 0; i < sup->num_deadlines; i++) {
		const struct task_wdt_deadline *dl = &sup->deadlines[i];

		if (dl->start >= sup->num_checkpoints || dl->end >= sup->num_checkpoints ||
		    (dl->max_ms != 0U && dl->min_ms >= dl->max_ms)) {
			return false;
		}
	}

	return true;
}

int task_wdt_supervision_start(struct task_wdt_supervision *sup, int channel_id)
{
	k_spinlock_key_t key;
	int ret;

	if (sup == NULL || !deadlines_valid(sup)) {
		return -EINVAL;
	}

	key = task_wdt_lock();

	if (sup->channel_id >= 0) {
		ret = -EBUSY;
	} else {
		ret = task_wdt_channel_sup_attach(channel_id, sup);
		if (ret == 0) {
			sup->last = sup->num_checkpoints;
			sup->active = sup->num_deadlines;
		}
	}

	task_wdt_unlock(key);

	return ret;
}

int task_wdt_supervision_stop(struct task_wdt_supervision *sup)
{
	k_spinlock_key_t key;
	int ret = 0;

	if (sup == NULL) {
		return -EINVAL;
	}

	key = task_wdt_lock();

	if (sup->channel_id < 0) {
		ret = -EINVAL;
	} else {
		task_wdt_channel_sup_detach(sup->channel_id);
	}

	task_wdt_unlock(key);

	return ret;
}

int task_wdt_checkpoint(struct task_wdt_supervision *sup, uint32_t checkpoint)
{
	enum task_wdt_fault_reason reason = TASK_WDT_FAULT_SEQUENCE;
	bool fault = false;
	k_spinlock_key_t key;
	int64_t current_ticks;
	int channel_id;

	if (sup == NULL || checkpoint >= sup->num_checkpoints) {
		return -EINVAL;
	}

	key = task_wdt_lock();

	channel_id = sup->channel_id;
	if (channel_id < 0) {
		task_wdt_unlock(key);
		return -EINVAL;
	}

	current_ticks = sys_clock_tick_get();

	/* order check, any checkpoint may be reached first */
	if (sup->last < sup->num_checkpoints && (sup->next[sup->last] & BIT(checkpoint)) == 0U) {
		reason = TASK_WDT_FAULT_SEQUENCE;
		fault = true;
	}

	/* end the running deadline */
	if (sup->active < sup->num_deadlines && sup->deadlines[sup->active].end == checkpoint) {
		const struct task_wdt_deadline *dl = &sup->deadlines[sup->active];

		if (!fault && dl->min_ms != 0U &&
		    current_ticks - sup->start_ticks < k_ms_to_ticks_ceil64(dl->min_ms)) {
			reason = TASK_WDT_FAULT_DEADLINE_MIN;
			fault = true;
		}

		sup->active = sup->num_deadlines;
		task_wdt_channel_deadline_set(channel_id, INT64_MAX);
	}

	sup->last = checkpoint;

	/* start a deadline, unless one is already running */
	if (sup->active == sup->num_deadlines) {
		for (size_t i = 0; i < sup->num_deadlines; i++) {
			const struct task_wdt_deadline *dl = &sup->deadlines[i];

			if (dl->start != checkpoint) {
				continue;
			}

			sup->active = i;
			sup->start_ticks = current_ticks;
			if (dl->max_ms != 0U) {
				task_wdt_channel_deadline_set(
					channel_id,
					current_ticks + k_ms_to_ticks_ceil64(dl->max_ms));
			}
			break;
		}
	}

	task_wdt_unlock(key);

	if (fault) {
		task_wdt_channel_fault(sup, channel_id, reason);
	}

	return 0;
}
