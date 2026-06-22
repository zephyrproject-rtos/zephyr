/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LATENCY_MEASURE_UNIT_H
#define _LATENCY_MEASURE_UNIT_H
/*
 * @brief This file contains function declarations, macros and inline functions
 * used in latency measurement.
 */

#include <zephyr/timing/timing.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <zephyr/timestamp.h>
#include <zephyr/app_memory/app_memdomain.h>

#define START_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define ALT_STACK_SIZE   (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#ifdef CONFIG_USERSPACE
#define  BENCH_BMEM  K_APP_BMEM(bench_mem_partition)
#else
#define  BENCH_BMEM
#endif

struct timestamp_data {
	uint64_t  cycles;
	timing_t  sample;
};

K_THREAD_STACK_DECLARE(start_stack, START_STACK_SIZE);
K_THREAD_STACK_DECLARE(alt_stack, ALT_STACK_SIZE);

extern struct k_thread start_thread;
extern struct k_thread alt_thread;

extern struct k_sem  pause_sem;

extern struct timestamp_data  timestamp;
#ifdef CONFIG_USERSPACE
extern uint64_t user_timestamp_overhead;
#endif
extern uint64_t timestamp_overhead;

extern int error_count;

#define TICK_OCCURRENCE_ERROR  "Error: Tick Occurred"

#ifdef CSV_FORMAT_OUTPUT
#define FORMAT_STR   "%-94s,%s,%s,%s\n"
#define CYCLE_FORMAT "%8u"
#define NSEC_FORMAT  "%8u"
#else
#define FORMAT_STR   "%-94s:%s , %s : %s\n"
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
