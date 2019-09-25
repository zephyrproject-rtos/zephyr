/* utils.h - utility functions used by latency measurement */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * This file contains function declarations, macroses and inline functions
 * used in latency measurement.
 */

#define INT_IMM8_OFFSET   1
#define IRQ_PRIORITY      3

#ifdef CONFIG_PRINTK
#include <sys/printk.h>
#include <stdio.h>
#include "timestamp.h"
extern char tmp_string[];
extern int error_count;

#define TMP_STRING_SIZE  100

#define PRINT(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)

#define PRINT_FORMAT(fmt, ...)						\
	do {                                                            \
		snprintf(tmp_string, TMP_STRING_SIZE, fmt, ##__VA_ARGS__); \
		PRINTF("|%-77s|\n", tmp_string);				\
	} while (0)

/**
 *
 * @brief Print dash line
 *
 * @return N/A
 */
static inline void print_dash_line(void)
{
	PRINTF("|-------------------------------------------------------"
	       "----------------------|\n");
}

#define PRINT_END_BANNER()						\
	do {								\
	PRINTF("|                                    E N D             " \
	       "                       |\n");				\
	print_dash_line();						\
	} while (0)


#define PRINT_BANNER()						\
	do {								\
	print_dash_line();						\
	PRINTF("|                            Latency Benchmark         " \
	       "                       |\n");				\
	print_dash_line();						\
	} while (0)


#define PRINT_TIME_BANNER()						\
	do {								\
	PRINT_FORMAT("  tcs = timer clock cycles: 1 tcs is %u nsec",	\
		     SYS_CLOCK_HW_CYCLES_TO_NS(1));			\
	print_dash_line();						\
	} while (0)

#define PRINT_OVERFLOW_ERROR()			\
	PRINT_FORMAT(" Error: tick occurred")

#else
#error PRINTK configuration option needs to be enabled
#endif

void raiseIntFunc(void);
extern void raiseInt(u8_t id);

/* pointer to the ISR */
typedef void (*ptestIsr) (void *unused);

