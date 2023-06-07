/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LATENCY_MEASURE_UNIT_H
#define _LATENCY_MEASURE_UNIT_H
/*
 * @brief This file contains function declarations, macroses and inline functions
 * used in latency measurement.
 */

#include <zephyr/timing/timing.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <zephyr/timestamp.h>

#define INT_IMM8_OFFSET   1
#define IRQ_PRIORITY      3

extern int error_count;

#define PRINT_OVERFLOW_ERROR()			\
	printk(" Error: tick occurred\n")

#ifdef CSV_FORMAT_OUTPUT
#define FORMAT_STR   "%-60s,%s,%s\n"
#define CYCLE_FORMAT "%8u"
#define NSEC_FORMAT  "%8u"
#else
#define FORMAT_STR   "%-60s:%s , %s\n"
#define CYCLE_FORMAT "%8u cycles"
#define NSEC_FORMAT  "%8u ns"
#endif

#define PRINT_F(summary, cycles, nsec, error, notes)                     \
	do {                                                             \
		char cycle_str[32];                                      \
		char nsec_str[32];                                       \
									 \
		if (!error) {                                            \
			snprintk(cycle_str, 30, CYCLE_FORMAT, cycles);   \
			snprintk(nsec_str, 30, NSEC_FORMAT, nsec);       \
		} else {                                                 \
			snprintk(cycle_str, 30, "%15s", "FAILED");       \
			snprintk(nsec_str, 30, "%15s", "FAILED");        \
		}                                                        \
		printk(FORMAT_STR, summary, cycle_str, nsec_str, notes); \
	} while (0)

#define PRINT_STATS(summary, value) \
	PRINT_F(summary, value, (uint32_t)timing_cycles_to_ns(value))

#define PRINT_STATS_AVG(summary, value, counter)	\
	PRINT_F(summary, value / counter, (uint32_t)timing_cycles_to_ns_avg(value, counter));


#endif
