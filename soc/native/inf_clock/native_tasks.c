/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Run the set of special native tasks corresponding to the given level
 *
 * @param level One of _NATIVE_*_LEVEL as defined in soc.h
 */
#ifdef __APPLE__

#include <zephyr/sys/util.h>

#include "posix_native_task.h"

/* ld64 orders the task entries by priority; markers delimit each task level. */
#define NATIVE_TASK_MARKERS(level) \
	static void (*const _CONCAT(__native_task_range_start_, level))() \
	__used __noasan NATIVE_TASK_SECTION(level, 0) = NULL; \
	static void (*const _CONCAT(__native_task_range_end_, level))() \
	__used __noasan NATIVE_TASK_SECTION(level, 0) = NULL

NATIVE_TASK_MARKERS(PRE_BOOT_1);
NATIVE_TASK_MARKERS(PRE_BOOT_2);
NATIVE_TASK_MARKERS(PRE_BOOT_3);
NATIVE_TASK_MARKERS(FIRST_SLEEP);
NATIVE_TASK_MARKERS(ON_EXIT);

struct native_task_range {
	void (*const *start)(void);
	void (*const *end)(void);
};

#define NATIVE_TASK_RANGE(level) \
	{ \
		&_CONCAT(__native_task_range_start_, level), \
		&_CONCAT(__native_task_range_end_, level), \
	}

void run_native_tasks(int level)
{
	static const struct native_task_range ranges[] = {
		NATIVE_TASK_RANGE(PRE_BOOT_1),
		NATIVE_TASK_RANGE(PRE_BOOT_2),
		NATIVE_TASK_RANGE(PRE_BOOT_3),
		NATIVE_TASK_RANGE(FIRST_SLEEP),
		NATIVE_TASK_RANGE(ON_EXIT),
	};

	if ((level < 0) || (level >= (int)ARRAY_SIZE(ranges))) {
		return;
	}

	for (void (*const *fptr)(void) = ranges[level].start; fptr < ranges[level].end;
	     fptr++) {
		if (*fptr != NULL) {
			(*fptr)();
		}
	}
}

#else

void run_native_tasks(int level)
{
	extern void (*__native_PRE_BOOT_1_tasks_start[])(void);
	extern void (*__native_PRE_BOOT_2_tasks_start[])(void);
	extern void (*__native_PRE_BOOT_3_tasks_start[])(void);
	extern void (*__native_FIRST_SLEEP_tasks_start[])(void);
	extern void (*__native_ON_EXIT_tasks_start[])(void);
	extern void (*__native_tasks_end[])(void);

	static void (**native_pre_tasks[])(void) = {
		__native_PRE_BOOT_1_tasks_start,
		__native_PRE_BOOT_2_tasks_start,
		__native_PRE_BOOT_3_tasks_start,
		__native_FIRST_SLEEP_tasks_start,
		__native_ON_EXIT_tasks_start,
		__native_tasks_end
	};

	void (**fptr)(void);

	for (fptr = native_pre_tasks[level]; fptr < native_pre_tasks[level+1];
		fptr++) {
		if (*fptr) { /* LCOV_EXCL_BR_LINE */
			(*fptr)();
		}
	}
}

#endif
