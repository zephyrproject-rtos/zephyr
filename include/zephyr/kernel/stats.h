/*
 * Copyright (c) 2021,2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_STATS_H_
#define ZEPHYR_INCLUDE_KERNEL_STATS_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * Structure used to track internal statistics about both thread
 * and CPU usage.
 */

struct k_cycle_stats {
	uint64_t  total;        /**< total usage in cycles */
#if defined(CONFIG_CPU_WORKLOAD_THREAD_PROFILE) || defined(__DOXYGEN__)
	uint64_t  burst_current;	/**< cycles accumulated in the current burst */
	uint32_t  burst_avg;		/**< EWMA of completed thread bursts in cycles */
	uint16_t  burst_samples;	/**< number of completed burst samples */
	uint8_t   burst_confidence;	/**< burst profile confidence, 0 to 100 */
#endif /* CONFIG_CPU_WORKLOAD_THREAD_PROFILE */
#if defined(CONFIG_CPU_WORKLOAD_ARRIVAL_PROFILE) || defined(__DOXYGEN__)
	uint64_t  arrival_cycles;	/**< expected cycles from attributed arrivals */
	uint32_t  arrival_source_mask;	/**< attributed arrival source bits */
	uint16_t  arrival_count;	/**< number of attributed arrivals */
	uint16_t  arrival_profiled;	/**< arrivals backed by burst profiles */
	uint8_t   arrival_confidence;	/**< arrival profile confidence, 0 to 100 */
#endif /* CONFIG_CPU_WORKLOAD_ARRIVAL_PROFILE */
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
