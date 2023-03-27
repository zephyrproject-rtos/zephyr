/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _POSIX_SOC_INF_CLOCK_SOC_H
#define _POSIX_SOC_INF_CLOCK_SOC_H

#include <zephyr/toolchain.h>
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
#define NATIVE_TASK(fn, level, prio)	\
	static void (* const _CONCAT(__native_task_, fn))() __used __noasan \
	__attribute__((__section__(".native_" #level STRINGIFY(prio) "_task")))\
	= fn


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
