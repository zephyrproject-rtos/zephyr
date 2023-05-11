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
#if defined(CONFIG_SCHED_THREAD_USAGE_ANALYSIS) || defined(__DOXYGEN__)
	/**
	 * @name Fields available when CONFIG_SCHED_THREAD_USAGE_ANALYSIS is selected.
	 * @{
	 */
	uint64_t  current;      /**< \# of cycles in current usage window */
	uint64_t  longest;      /**< \# of cycles in longest usage window */
	uint32_t  num_windows;  /**< \# of usage windows */
	/** @} */
#endif
	bool      track_usage;  /**< true if gathering usage stats */
};

/**
 * @brief Return the cumulative number of cycles used in system calls.
 *
 * @param thread The thread to query system call stats for, or `NULL` for all threads.
 * @return uint64_t
 */
uint64_t k_syscall_cycle_stats(struct k_thread *thread);

#endif
