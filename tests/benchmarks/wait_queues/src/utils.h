/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BENCHMARK_WAITQ_UTILS_H
#define __BENCHMARK_WAITQ_UTILS_H
/*
 * @brief This file contains macros used in the wait queue benchmarking.
 */

#include <zephyr/sys/printk.h>

#ifdef CSV_FORMAT_OUTPUT
#define FORMAT_STR   "%-74s,%s,%s\n"
#define CYCLE_FORMAT "%8u"
#define NSEC_FORMAT  "%8u"
#else
#define FORMAT_STR   "%-74s:%s , %s\n"
#define CYCLE_FORMAT "%8u cycles"
#define NSEC_FORMAT  "%8u ns"
#endif

/**
 * @brief Display a line of statistics
 *
 * This macro displays the following:
 *  1. Test description summary
 *  2. Number of cycles
 *  3. Number of nanoseconds
 */
#define PRINT_F(summary, cycles, nsec)                                   \
	do {                                                             \
		char cycle_str[32];                                      \
		char nsec_str[32];                                       \
									 \
		snprintk(cycle_str, 30, CYCLE_FORMAT, cycles);           \
		snprintk(nsec_str, 30, NSEC_FORMAT, nsec);               \
		printk(FORMAT_STR, summary, cycle_str, nsec_str);        \
	} while (0)

#define PRINT_STATS_AVG(summary, value, counter)                    \
	PRINT_F(summary, value / counter,                           \
		(uint32_t)timing_cycles_to_ns_avg(value, counter))

#endif
