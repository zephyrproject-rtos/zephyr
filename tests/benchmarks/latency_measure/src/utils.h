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
#include "timestamp.h"

#define INT_IMM8_OFFSET   1
#define IRQ_PRIORITY      3

extern int error_count;

#define PRINT_OVERFLOW_ERROR()			\
	printk(" Error: tick occurred\n")

#ifdef CSV_FORMAT_OUTPUT
#define FORMAT "%-60s,%8u,%8u\n"
#else
#define FORMAT "%-60s:%8u cycles , %8u ns\n"
#endif

#define PRINT_F(...)						\
	{							\
		char sline[256]; 				\
		snprintk(sline, 254, FORMAT, ##__VA_ARGS__); 	\
		printk("%s", sline);			     	\
	}

#define PRINT_STATS(x, y) \
	PRINT_F(x, y, (uint32_t)timing_cycles_to_ns(y))

#define PRINT_STATS_AVG(x, y, counter)	\
	PRINT_F(x, y / counter, (uint32_t)timing_cycles_to_ns_avg(y, counter));


#endif
