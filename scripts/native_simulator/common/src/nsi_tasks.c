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
