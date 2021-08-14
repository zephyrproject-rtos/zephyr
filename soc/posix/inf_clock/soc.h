/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _POSIX_SOC_INF_CLOCK_SOC_H
#define _POSIX_SOC_INF_CLOCK_SOC_H

#include <toolchain.h>
#include "board_soc.h"
#include "posix_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

void posix_soc_clean_up(void);

/**
 * NATIVE_TASK
 *
 * For native_posix, register a function to be called at particular moments
 * during the native_posix execution.
 *
 * There is 5 choices for when the function will be called (level):
 * * PRE_BOOT_1: Will be called before the command line parameters are parsed,
 *   or the HW models are initialized
 *
 * * PRE_BOOT_2: Will be called after the command line parameters are parsed,
 *   but before the HW models are initialized
 *
 * * PRE_BOOT_3: Will be called after the HW models initialization, right before
 *   the "CPU is booted" and Zephyr is started.
 *
 * * FIRST_SLEEP: Will be called the 1st time the CPU is sent to sleep
 *
 * * ON_EXIT: Will be called during termination of the native application
 * execution.
 *
 * The function must take no parameters and return nothing.
 * Note that for the PRE and ON_EXIT levels neither the Zephyr kernel or
 * any Zephyr thread are running.
 */
#if defined(__APPLE__) && defined(__MACH__)
#define NATIVE_TASK(fn, level, prio)	\
	__attribute__((constructor)) \
	static void _CONCAT(__ctor_, fn)(void) { \
		extern void __z_native_posix_task_add(const char *n, \
			const void *f, const char *l, int p); \
		__z_native_posix_task_add(STRINGIFY(fn), fn, STRINGIFY(level), \
			prio); \
	}
#else
#define NATIVE_TASK(fn, level, prio)	\
	static void (*_CONCAT(__native_task_, fn)) __used	\
	__z_section(".native_" #level STRINGIFY(prio) "_task")\
	= fn
#endif /* defined(__APPLE__) && defined(__MACH__) */


#define _NATIVE_PRE_BOOT_1_LEVEL	0
#define _NATIVE_PRE_BOOT_2_LEVEL	1
#define _NATIVE_PRE_BOOT_3_LEVEL	2
#define _NATIVE_FIRST_SLEEP_LEVEL	3
#define _NATIVE_ON_EXIT_LEVEL		4

/**
 * @brief Run the set of special native tasks corresponding to the given level
 *
 * @param level One of _NATIVE_*_LEVEL as defined in soc.h
 */
void run_native_tasks(int level);

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_SOC_INF_CLOCK_SOC_H */
