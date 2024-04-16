/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_NSI_TASKS_H
#define NSI_COMMON_SRC_NSI_TASKS_H

#include "nsi_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NSITASK_PRE_BOOT_1_LEVEL	0
#define NSITASK_PRE_BOOT_2_LEVEL	1
#define NSITASK_HW_INIT_LEVEL		2
#define NSITASK_PRE_BOOT_3_LEVEL	3
#define NSITASK_FIRST_SLEEP_LEVEL	4
#define NSITASK_ON_EXIT_PRE_LEVEL	5
#define NSITASK_ON_EXIT_POST_LEVEL	6

/**
 * NSI_TASK
 *
 * Register a function to be called at particular moments
 * during the Native Simulator execution.
 *
 * There is 5 choices for when the function will be called (level):
 * * PRE_BOOT_1: Will be called before the command line parameters are parsed,
 *   or the HW models are initialized
 *
 * * PRE_BOOT_2: Will be called after the command line parameters are parsed,
 *   but before the HW models are initialized
 *
 * * HW_INIT: Will be called during HW models initialization
 *
 * * PRE_BOOT_3: Will be called after the HW models initialization, right before
 *   the "CPUs are booted" and embedded SW in them is started.
 *
 * * FIRST_SLEEP: Will be called after the 1st time all CPUs are sent to sleep
 *
 * * ON_EXIT_PRE: Will be called during termination of the runner
 * execution, as a first set.
 *
 * * ON_EXIT_POST: Will be called during termination of the runner
 * execution, as the very last set before the program returns.
 *
 * The function must take no parameters and return nothing.
 */
#define NSI_TASK(fn, level, prio)	\
	static void (* const NSI_CONCAT(__nsi_task_, fn))(void) \
	__attribute__((__used__)) \
	__attribute__((__section__(".nsi_" #level NSI_STRINGIFY(prio) "_task")))\
	= fn; \
	/* Let's cross-check the macro level is a valid one, so we don't silently drop it */ \
	_Static_assert(NSITASK_##level##_LEVEL >= 0, \
			"Using a non pre-defined level, it will be dropped")

/**
 * @brief Run the set of special native tasks corresponding to the given level
 *
 * @param level One of NSITASK_*_LEVEL as defined in soc.h
 */
void nsi_run_tasks(int level);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_NSI_TASKS_H */
