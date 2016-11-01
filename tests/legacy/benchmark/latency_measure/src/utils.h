/* utils.h - utility functions used by latency measurement */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * DESCRIPTION
 * This file contains function declarations, macroses and inline functions
 * used in latency measurement.
 */

#define INT_IMM8_OFFSET   1
#define IRQ_PRIORITY      3

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#include <stdio.h>
#include "timestamp.h"
extern char tmpString[];
extern int errorCount;

#define TMP_STRING_SIZE  100

#define PRINT(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)

#define PRINT_FORMAT(fmt, ...)						\
	do {                                                            \
		snprintf(tmpString, TMP_STRING_SIZE, fmt, ##__VA_ARGS__); \
		PRINTF("|%-77s|\n", tmpString);				\
	} while (0)

/**
 *
 * @brief Print dash line
 *
 * @return N/A
 */
static inline void printDashLine(void)
{
	PRINTF("|-------------------------------------------------------"
	       "----------------------|\n");
}

#define PRINT_END_BANNER()						\
	do {								\
	PRINTF("|                                    E N D             " \
	       "                       |\n");				\
	printDashLine();						\
	} while (0)

#define PRINT_NANO_BANNER()						\
	do {								\
	printDashLine();						\
	PRINTF("|                        Nanokernel Latency Benchmark  " \
	       "                       |\n");				\
	printDashLine();						\
	} while (0)

#define PRINT_MICRO_BANNER()						\
	do {								\
	printDashLine();						\
	PRINTF("|                        Microkernel Latency Benchmark " \
	       "                       |\n");				\
	printDashLine();						\
	} while (0)


#define PRINT_TIME_BANNER()						\
	do {								\
	PRINT_FORMAT("  tcs = timer clock cycles: 1 tcs is %lu nsec",	\
		     SYS_CLOCK_HW_CYCLES_TO_NS(1));			\
	printDashLine();						\
	} while (0)

#define PRINT_OVERFLOW_ERROR()			\
	PRINT_FORMAT(" Error: tick occurred")

#else
#error PRINTK configuration option needs to be enabled
#endif

void raiseIntFunc(void);
extern void raiseInt(uint8_t id);

/* test the interrupt latency */
int nanoIntLatency(void);
int nanoIntToFiber(void);
int nanoIntToFiberSem(void);
int nanoCtxSwitch(void);
int nanoIntLockUnlock(void);

/* pointer to the ISR */
typedef void (*ptestIsr) (void *unused);

