/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/cpu_workload/cpu_workload.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>

LOG_MODULE_REGISTER(cpu_workload_work, CONFIG_CPU_WORKLOAD_LOG_LEVEL);

#define CPU_WORKLOAD_WORK_PROFILE_CONFIDENCE_BASE 20U
#define CPU_WORKLOAD_WORK_PROFILE_CONFIDENCE_STEP 10U
#define CPU_WORKLOAD_WORK_PROFILE_CONFIDENCE_MAX 80U

struct cpu_workload_work_profile_slot {
	struct k_work *work;
	uint64_t observed_total_cycles;
	uint16_t observed_count;
	uint16_t sample_count; /* CPU workload profile maturity count */
	uint32_t handler_avg_cycles;
};

static struct k_spinlock work_profile_lock;

/**
 * This is a simple fixed-size cache of work handler profiles. It saves the state
 * of the work from the last time cpu_workload observed it, along with the EWMA
 * that it calculated itself.
 */
static struct cpu_workload_work_profile_slot
	work_profile_slots[CONFIG_CPU_WORKLOAD_WORK_PROFILE_CACHE_SIZE];

static struct cpu_workload_work_profile_slot *work_profile_slot_get(struct k_work *work)
{
	struct cpu_workload_work_profile_slot *free_slot = NULL;

	for (size_t slot_index = 0U;
	     slot_index < ARRAY_SIZE(work_profile_slots); slot_index++) {
		if (work_profile_slots[slot_index].work == work) {
			return &work_profile_slots[slot_index];
		}

		if ((free_slot == NULL) && (work_profile_slots[slot_index].work == NULL)) {
			free_slot = &work_profile_slots[slot_index];
		}
	}

	if (free_slot != NULL) {
		free_slot->work = work;
	}

	return free_slot;
}

/**
 * Update EWMA with incremental average
 */
static void work_profile_update_avg(struct cpu_workload_work_profile_slot *slot,
				    uint32_t handler_cycles)
{
	uint32_t avg = slot->handler_avg_cycles;

	/* If there is no history, start with the current observed average.
	 * Otherwise, update the EWMA with the new observed average.
	 *
	 * CONFIG_CPU_WORKLOAD_WORK_PROFILE_EWMA_SHIFT controls the response speed:
	 * Smaller shift → faster tracking of new samples
	 * Larger shift → smoother averaging
	 */
	if (avg == 0U) {
		slot->handler_avg_cycles = handler_cycles;
	} else if (handler_cycles >= avg) {
		slot->handler_avg_cycles = avg +
			((handler_cycles - avg) >> CONFIG_CPU_WORKLOAD_WORK_PROFILE_EWMA_SHIFT);
	} else {
		slot->handler_avg_cycles = avg -
			((avg - handler_cycles) >> CONFIG_CPU_WORKLOAD_WORK_PROFILE_EWMA_SHIFT);
	}
}

static uint8_t work_profile_confidence(uint16_t sample_count)
{
	uint32_t confidence;

	/* No samples: confidence = 0 */
	if (sample_count == 0U) {
		return 0U;
	}

	/* Few samples: start from 20; More samples = more reliable;
	 * Maximum 80, remain conservative.
	 */
	confidence = CPU_WORKLOAD_WORK_PROFILE_CONFIDENCE_BASE +
		(sample_count * CPU_WORKLOAD_WORK_PROFILE_CONFIDENCE_STEP);

	return (uint8_t)MIN(confidence, CPU_WORKLOAD_WORK_PROFILE_CONFIDENCE_MAX);
}

int cpu_workload_work_get(struct k_work *work, struct cpu_workload_work_profile *profile)
{
	struct cpu_workload_work_profile_slot *slot;
	k_work_handler_runtime_stats_t stats;
	k_spinlock_key_t key;
	uint64_t cycle_delta;
	uint32_t handler_cycles;
	uint16_t count_delta;
	int ret;

	CHECKIF((work == NULL) || (profile == NULL)) {
		return -EINVAL;
	}

	ret = k_work_handler_runtime_stats_get(work, &stats);
	if (ret != 0) {
		return ret;
	}

	key = k_spin_lock(&work_profile_lock);
	slot = work_profile_slot_get(work);
	if (slot == NULL) {
		k_spin_unlock(&work_profile_lock, key);
		return -ENOMEM;
	}

	/* Under normal circumstances, the kernel's count and total_cycles should increase
	 * monotonically. If the current value is smaller than the previous observed value,
	 * clear the cpu_workload's own slot state and restart the modeling.
	 */
	if ((stats.count < slot->observed_count) ||
	    (stats.total_cycles < slot->observed_total_cycles)) {
		slot->observed_total_cycles = 0U;
		slot->observed_count = 0U;
		slot->sample_count = 0U;
		slot->handler_avg_cycles = 0U;
	}

	/* Calculate the incremental values for this round:
	 * count_delta: How many times has this handler executed since the last call
	 * cycle_delta: How many cycles in total have been consumed by these new executions
	 */
	count_delta = stats.count - slot->observed_count;
	cycle_delta = stats.total_cycles - slot->observed_total_cycles;
	if ((count_delta != 0U) && (cycle_delta != 0U)) {
		handler_cycles = (uint32_t)MIN(cycle_delta / count_delta, (uint64_t)UINT32_MAX);
		/* Update EWMA with incremental average */
		work_profile_update_avg(slot, handler_cycles);

		if ((UINT16_MAX - slot->sample_count) < count_delta) {
			slot->sample_count = UINT16_MAX;
		} else {
			slot->sample_count += count_delta;
		}

		slot->observed_total_cycles = stats.total_cycles;
		slot->observed_count = stats.count;
	}

	profile->handler_cycles = MAX(stats.last_cycles, slot->handler_avg_cycles);
	profile->source_mask = (slot->sample_count == 0U) ? 0U :
		CPU_WORKLOAD_SOURCE_WORK_PROFILE;
	profile->sample_count = slot->sample_count;
	profile->confidence = work_profile_confidence(slot->sample_count);

	k_spin_unlock(&work_profile_lock, key);

	LOG_DBG("work %p handler profile: cycles=%u samples=%u confidence=%u",
		work, profile->handler_cycles, profile->sample_count, profile->confidence);

	return 0;
}
