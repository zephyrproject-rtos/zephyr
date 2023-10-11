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

#define TICK_OCCURRENCE_ERROR  "Error: Tick Occurred"

#ifdef CSV_FORMAT_OUTPUT
#define FORMAT_STR   "%-52s,%s,%s,%s\n"
#define CYCLE_FORMAT "%8u"
#define NSEC_FORMAT  "%8u"
#else
#define FORMAT_STR   "%-52s:%s , %s : %s\n"
#define CYCLE_FORMAT "%8u cycles"
#define NSEC_FORMAT  "%8u ns"
#endif

/**
 * @brief Display a line of statistics
 *
 * This macro displays the following:
 *  1. Test description summary
 *  2. Number of cycles - See Note
 *  3. Number of nanoseconds - See Note
 *  4. Additional notes describing the nature of any errors
 *
 * Note - If the @a error parameter is not false, then the test has no
 * numerical information to print and it will instead print "FAILED".
 */
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

#define PRINT_STATS(summary, value, error, notes)     \
	PRINT_F(summary, value,                       \
		(uint32_t)timing_cycles_to_ns(value), \
		error, notes)

#define PRINT_STATS_AVG(summary, value, counter, error, notes)      \
	PRINT_F(summary, value / counter,                           \
		(uint32_t)timing_cycles_to_ns_avg(value, counter),  \
		error, notes);


#endif
