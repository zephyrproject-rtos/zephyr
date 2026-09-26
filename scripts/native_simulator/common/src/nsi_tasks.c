/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Run the set of special NSI tasks corresponding to the given level
 *
 * @param level One of NSITASK_*_LEVEL as defined in nsi_tasks.h
 */
#ifdef __APPLE__

#include <stddef.h>

#include "nsi_tasks.h"

/* Keep bounds materialized and ordered for levels without registered tasks. */
#define NSI_TASK_MARKERS(level) \
	void (*const NSI_CONCAT(__nsi_task_range_start_, level))(void) \
	NSI_SECTION_DATA(NSI_TASK_MACHO_SEC_LEVEL(level)) = NULL; \
	void (*const NSI_CONCAT(__nsi_task_range_end_, level))(void) \
	NSI_SECTION_DATA(NSI_TASK_MACHO_SEC_LEVEL(level)) = NULL

NSI_TASK_MARKERS(PRE_BOOT_1);
NSI_TASK_MARKERS(PRE_BOOT_2);
NSI_TASK_MARKERS(HW_INIT);
NSI_TASK_MARKERS(PRE_BOOT_3);
NSI_TASK_MARKERS(FIRST_SLEEP);
NSI_TASK_MARKERS(ON_EXIT_PRE);
NSI_TASK_MARKERS(ON_EXIT_POST);

struct nsi_task_range {
	void (**start)(void);
	void (**end)(void);
};

#define NSI_TASK_RANGE(level) \
	{ __nsi_##level##_tasks_start, __nsi_##level##_tasks_end }

extern void (*__nsi_PRE_BOOT_1_tasks_start[])(void);
extern void (*__nsi_PRE_BOOT_1_tasks_end[])(void);
extern void (*__nsi_PRE_BOOT_2_tasks_start[])(void);
extern void (*__nsi_PRE_BOOT_2_tasks_end[])(void);
extern void (*__nsi_HW_INIT_tasks_start[])(void);
extern void (*__nsi_HW_INIT_tasks_end[])(void);
extern void (*__nsi_PRE_BOOT_3_tasks_start[])(void);
extern void (*__nsi_PRE_BOOT_3_tasks_end[])(void);
extern void (*__nsi_FIRST_SLEEP_tasks_start[])(void);
extern void (*__nsi_FIRST_SLEEP_tasks_end[])(void);
extern void (*__nsi_ON_EXIT_PRE_tasks_start[])(void);
extern void (*__nsi_ON_EXIT_PRE_tasks_end[])(void);
extern void (*__nsi_ON_EXIT_POST_tasks_start[])(void);
extern void (*__nsi_ON_EXIT_POST_tasks_end[])(void);

void nsi_run_tasks(int level)
{
	static const struct nsi_task_range ranges[] = {
		NSI_TASK_RANGE(PRE_BOOT_1),
		NSI_TASK_RANGE(PRE_BOOT_2),
		NSI_TASK_RANGE(HW_INIT),
		NSI_TASK_RANGE(PRE_BOOT_3),
		NSI_TASK_RANGE(FIRST_SLEEP),
		NSI_TASK_RANGE(ON_EXIT_PRE),
		NSI_TASK_RANGE(ON_EXIT_POST),
	};
	void (**fptr)(void);

	if ((level < 0) || (level >= (int)NSI_ARRAY_SIZE(ranges))) {
		return;
	}

	for (fptr = ranges[level].start; fptr < ranges[level].end; fptr++) {
		if (*fptr != NULL) {
			(*fptr)();
		}
	}
}

#else

void nsi_run_tasks(int level)
{
	extern void (*__nsi_PRE_BOOT_1_tasks_start[])(void);
	extern void (*__nsi_PRE_BOOT_2_tasks_start[])(void);
	extern void (*__nsi_HW_INIT_tasks_start[])(void);
	extern void (*__nsi_PRE_BOOT_3_tasks_start[])(void);
	extern void (*__nsi_FIRST_SLEEP_tasks_start[])(void);
	extern void (*__nsi_ON_EXIT_PRE_tasks_start[])(void);
	extern void (*__nsi_ON_EXIT_POST_tasks_start[])(void);
	extern void (*__nsi_tasks_end[])(void);

	static void (**nsi_pre_tasks[])(void) = {
		__nsi_PRE_BOOT_1_tasks_start,
		__nsi_PRE_BOOT_2_tasks_start,
		__nsi_HW_INIT_tasks_start,
		__nsi_PRE_BOOT_3_tasks_start,
		__nsi_FIRST_SLEEP_tasks_start,
		__nsi_ON_EXIT_PRE_tasks_start,
		__nsi_ON_EXIT_POST_tasks_start,
		__nsi_tasks_end
	};

	void (**fptr)(void);

	for (fptr = nsi_pre_tasks[level]; fptr < nsi_pre_tasks[level+1];
		fptr++) {
		if (*fptr) { /* LCOV_EXCL_BR_LINE */
			(*fptr)();
		}
	}
}

#endif
