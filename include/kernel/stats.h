/*
 * Copyright (c) 2021, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_STATS_H_
#define ZEPHYR_INCLUDE_KERNEL_STATS_H_

#include <stdint.h>

/*
 * [k_cycle_stats] is used to track internal statistics about both thread
 * and CPU usage.
 */

struct k_cycle_stats {
	uint64_t  total;        /* total usage in cycles */
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	uint64_t  current;      /* # of cycles in current usage window */
	uint64_t  longest;      /* # of cycles in longest usage window */
	uint32_t  num_windows;  /* # of usage windows */
#endif
	bool      track_usage;  /* true if gathering usage stats */
};

#endif
