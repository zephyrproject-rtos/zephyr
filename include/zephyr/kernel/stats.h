/*
 * Copyright (c) 2021,2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_STATS_H_
#define ZEPHYR_INCLUDE_KERNEL_STATS_H_

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/sys/util.h>

/**
 * Source bits for per-thread scheduler arrival statistics.
 *
 * An arrival is counted when scheduler code records that a tracked
 * thread is made runnable. The source bits identify the scheduler
 * path that recorded the arrival; they are accumulated with the
 * thread's usage statistics and reset with the same usage accounting
 * state.
 */
enum k_thread_arrival_source {
	/** Thread arrival came from a timeout expiry. */
	K_THREAD_ARRIVAL_SOURCE_TIMEOUT = BIT(0),

	/** Thread arrival came from a synchronization object becoming ready. */
	K_THREAD_ARRIVAL_SOURCE_SYNC = BIT(1),

	/** Thread arrival came from an explicit thread wakeup. */
	K_THREAD_ARRIVAL_SOURCE_EXPLICIT = BIT(2),
};

/**
 * Structure used to track internal statistics about both thread
 * and CPU usage.
 */

struct k_cycle_stats {
	uint64_t  total;        /**< total usage in cycles */
#if defined(CONFIG_SCHED_THREAD_USAGE_ARRIVAL_STATS) || defined(__DOXYGEN__)
	uint32_t  arrival_source_mask;	/**< attributed arrival source bits */
	uint16_t  arrival_count;	/**< number of attributed arrivals */
#endif /* CONFIG_SCHED_THREAD_USAGE_ARRIVAL_STATS */
#if defined(CONFIG_SCHED_THREAD_USAGE_ANALYSIS) || defined(__DOXYGEN__)
	/**
	 * @name Fields available when CONFIG_SCHED_THREAD_USAGE_ANALYSIS is selected.
	 * @{
	 */
	uint64_t  current;      /**< \# of cycles in current usage window */
	uint64_t  longest;      /**< \# of cycles in longest usage window */
	uint32_t  num_windows;  /**< \# of usage windows */
	/** @} */
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
	bool      track_usage;  /**< true if gathering usage stats */
};

#endif /* ZEPHYR_INCLUDE_KERNEL_STATS_H_ */
