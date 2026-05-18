/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CPU_WORKLOAD_INTERNAL_H_
#define ZEPHYR_SUBSYS_CPU_WORKLOAD_INTERNAL_H_

#include <stdint.h>

#include <zephyr/kernel.h>

/**
 * @brief Internal per-thread burst workload profile.
 *
 * The profile describes a thread's typical execution burst length based on
 * runtime-stat windows. It is consumed by cpu_workload signals that need a
 * per-thread cost estimate, such as ready-backlog and arrival attribution.
 */
struct cpu_workload_thread_burst_profile {
	/**
	 * EWMA of CPU cycles consumed per runtime-stat window.
	 *
	 * Each new sample is computed from the thread's runtime-stat cycle delta
	 * divided by its runtime-stat window-count delta since the previous query.
	 */
	uint32_t burst_avg_cycles;

	/**
	 * Number of runtime-stat windows observed for the thread.
	 *
	 * This is the thread's cumulative runtime-stat window count, not only the
	 * number of windows observed by the current query. It is used to derive the
	 * confidence score for the cached EWMA profile.
	 */
	uint32_t sample_count;

	/** Confidence in @ref burst_avg_cycles, from 0 to 100. */
	uint8_t confidence;
};

/**
 * @brief Get or update the internal burst profile for a thread.
 *
 * The query reads the thread's cumulative runtime statistics, updates the
 * cached EWMA profile if new runtime-stat windows are available, and writes
 * the current burst profile to @p profile.
 *
 * @param thread Thread for which to get the burst profile.
 * @param profile Pointer to the output burst profile.
 *
 * @retval 0 If the burst profile was written.
 * @retval -EINVAL If @p thread or @p profile is invalid.
 * @retval -ENOMEM If no cache slot is available for @p thread.
 */
int cpu_workload_thread_burst_profile_get(k_tid_t thread,
					  struct cpu_workload_thread_burst_profile *profile);

#endif /* ZEPHYR_SUBSYS_CPU_WORKLOAD_INTERNAL_H_ */
