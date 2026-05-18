/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>

#include "cpu_workload_internal.h"

#define CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_BASE 20U
#define CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_STEP 10U
#define CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_MAX 80U

struct cpu_workload_thread_profile_slot {
	k_tid_t thread;
	uint64_t observed_total_cycles;
	uint32_t observed_windows;
	uint32_t burst_avg_cycles;
};

static struct k_spinlock thread_profile_lock;
static struct cpu_workload_thread_profile_slot
	thread_profile_slots[CONFIG_CPU_WORKLOAD_THREAD_PROFILE_CACHE_SIZE];

static struct cpu_workload_thread_profile_slot *thread_profile_slot_get(k_tid_t thread)
{
	struct cpu_workload_thread_profile_slot *free_slot = NULL;

	for (size_t i = 0U; i < ARRAY_SIZE(thread_profile_slots); i++) {
		if (thread_profile_slots[i].thread == thread) {
			return &thread_profile_slots[i];
		}

		if ((free_slot == NULL) && (thread_profile_slots[i].thread == NULL)) {
			free_slot = &thread_profile_slots[i];
		}
	}

	if (free_slot != NULL) {
		free_slot->thread = thread;
	}

	return free_slot;
}

/**
 * Calculate the average CPU cycles consumed per scheduling window for a thread since the
 * last sampling, used to characterize the thread's burst execution behavior.
 */
static uint32_t thread_profile_window_delta(const k_thread_runtime_stats_t *stats,
					    struct cpu_workload_thread_profile_slot *slot)
{
	uint32_t window_delta;
	uint64_t cycle_delta;

	/* If the new window_count/total_cycles is less than the old ledger, then the
	 * old portrait can no longer be used and must be cleared and re-learned.
	 */
	if ((stats->window_count < slot->observed_windows) ||
	    (stats->total_cycles < slot->observed_total_cycles)) {
		slot->observed_windows = 0U;
		slot->observed_total_cycles = 0U;
		slot->burst_avg_cycles = 0U;
	}

	/* Calculate the average number of cycles this thread ran per window during
	 * this newly added observation period — (single burst length).
	 */
	window_delta = stats->window_count - slot->observed_windows;
	cycle_delta = stats->total_cycles - slot->observed_total_cycles;

	if (window_delta == 0U) {
		return 0U;
	}

	return (uint32_t)MIN(cycle_delta / window_delta, (uint64_t)UINT32_MAX);
}

/**
 * Use EWMA to merge the newly observed burst_cycles into the long-term maintained
 * burst_avg_cycles of the slot, so that the thread profile can both follow changes
 * in thread behavior and avoid being skewed by single-time fluctuations.
 */
static void thread_profile_update_avg(struct cpu_workload_thread_profile_slot *slot,
				      uint32_t burst_cycles)
{
	uint32_t avg = slot->burst_avg_cycles;

	/**
	 * 1. When avg == 0 (first sample), directly initialize with the sample value to avoid
	 *    starting from 0 and slowly climbing up, which would cause it to remain low for an
	 *    extended period.
	 *
	 * 2. burst_cycles >= avg (sample is larger than average), avg += (sample - avg) >> shift:
	 *    Uses unsigned subtraction to first calculate the positive difference, then
	 *    right-shifts it. This avoids implementation-defined behavior of signed
	 *    arithmetic / negative number right-shifting.
	 *
	 * 3. burst_cycles < avg (sample is smaller than average), avg -= (avg - sample) >> shift:
	 *    Similar to the above, but in reverse to decrease the average when the new sample is
	 *    lower.
	 */
	if (avg == 0U) {
		slot->burst_avg_cycles = burst_cycles;
	} else if (burst_cycles >= avg) {
		slot->burst_avg_cycles = avg +
			((burst_cycles - avg) >> CONFIG_CPU_WORKLOAD_THREAD_PROFILE_EWMA_SHIFT);
	} else {
		slot->burst_avg_cycles = avg -
			((avg - burst_cycles) >> CONFIG_CPU_WORKLOAD_THREAD_PROFILE_EWMA_SHIFT);
	}
}

/**
 * Map the currently accumulated sample count (sample_count) to a confidence score ranging
 * from 0 to 80, indicating how reliable the current burst_avg_cycles profile is. It is
 * metadata exposed externally together with burst_avg_cycles — the value itself + how much
 * reference value this value has.
 */
static uint8_t thread_profile_confidence(uint32_t sample_count)
{
	uint32_t confidence;

	/* If no samples have been collected, the confidence is 0. */
	if (sample_count == 0U) {
		return 0U;
	}

	/* For every additional CONFIG_CPU_WORKLOAD_THREAD_PROFILE_EWMA_SHIFT samples collected,
	 * the confidence increases by CONFIG_CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_STEP, until
	 * it reaches the maximum.
	 */
	if (sample_count >= ((CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_MAX -
			      CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_BASE) /
			      CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_STEP)) {
		return CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_MAX;
	}

	/* Reached sufficient samples threshold, capped */
	confidence = CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_BASE +
		     (sample_count * CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_STEP);

	return (uint8_t)MIN(confidence, CPU_WORKLOAD_THREAD_PROFILE_CONFIDENCE_MAX);
}

/**
 * Get the burst profile of a thread, including the average burst cycles, sample count,
 * and confidence.
 *
 * Uses the absolute cumulative window_count, not delta: this means that as long as a thread
 * has been scheduled enough times in its history, confidence will saturate; this complements
 * EWMA's semantics of 'continuously refreshing with new samples'.
 */
int cpu_workload_thread_burst_profile_get(k_tid_t thread,
					  struct cpu_workload_thread_burst_profile *profile)
{
	struct cpu_workload_thread_profile_slot *slot;
	k_thread_runtime_stats_t stats;
	uint32_t burst_cycles;
	k_spinlock_key_t key;
	int ret;

	CHECKIF((thread == NULL) || (profile == NULL)) {
		return -EINVAL;
	}

	ret = k_thread_runtime_stats_get(thread, &stats);
	if (ret != 0) {
		return ret;
	}

	key = k_spin_lock(&thread_profile_lock);
	slot = thread_profile_slot_get(thread);
	if (slot == NULL) {
		k_spin_unlock(&thread_profile_lock, key);
		return -ENOMEM;
	}

	burst_cycles = thread_profile_window_delta(&stats, slot);
	if (burst_cycles != 0U) {
		thread_profile_update_avg(slot, burst_cycles);
	}

	slot->observed_windows = stats.window_count;
	slot->observed_total_cycles = stats.total_cycles;

	profile->burst_avg_cycles = slot->burst_avg_cycles;
	profile->sample_count = stats.window_count;
	profile->confidence = thread_profile_confidence(stats.window_count);

	k_spin_unlock(&thread_profile_lock, key);

	return 0;
}
