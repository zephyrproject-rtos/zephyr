/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file for the native simulator runner
 */

#define NSI_INIT_LEVEL(level)					\
		__nsi_##level##_tasks_start = .;			\
		KEEP(*(SORT(.nsi_##level[0-9]_task)));		\
		KEEP(*(SORT(.nsi_##level[1-9][0-9]_task)));		\
		KEEP(*(SORT(.nsi_##level[1-9][0-9][0-9]_task)));	\

SECTIONS
 {
	nsi_tasks :
	{
		__nsi_tasks_start = .;
		NSI_INIT_LEVEL(PRE_BOOT_1)
		NSI_INIT_LEVEL(PRE_BOOT_2)
		NSI_INIT_LEVEL(HW_INIT)
		NSI_INIT_LEVEL(PRE_BOOT_3)
		NSI_INIT_LEVEL(FIRST_SLEEP)
		NSI_INIT_LEVEL(ON_EXIT_PRE)
		NSI_INIT_LEVEL(ON_EXIT_POST)
		__nsi_tasks_end = .;
	}

	nsi_hw_events :
	{
		__nsi_hw_events_start = .;
		KEEP(*(SORT(.nsi_hw_event_[0-9])));		\
		KEEP(*(SORT(.nsi_hw_event_[1-9][0-9])));	\
		KEEP(*(SORT(.nsi_hw_event_[1-9][0-9][0-9])));
		__nsi_hw_events_end = .;
	}
 } INSERT AFTER .data;

/*
 * Note this script augments the default host linker script
 */
