/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __APPLE__

#define MACHO_ALIAS(sym, target) \
	__asm__(".globl _" #sym "\n_" #sym " = " target)

/* Runner-side NSI task hooks */
MACHO_ALIAS(__nsi_PRE_BOOT_1_tasks_start, "___nsi_task_range_start_PRE_BOOT_1");
MACHO_ALIAS(__nsi_PRE_BOOT_1_tasks_end, "___nsi_task_range_end_PRE_BOOT_1");
MACHO_ALIAS(__nsi_PRE_BOOT_2_tasks_start, "___nsi_task_range_start_PRE_BOOT_2");
MACHO_ALIAS(__nsi_PRE_BOOT_2_tasks_end, "___nsi_task_range_end_PRE_BOOT_2");
MACHO_ALIAS(__nsi_HW_INIT_tasks_start, "___nsi_task_range_start_HW_INIT");
MACHO_ALIAS(__nsi_HW_INIT_tasks_end, "___nsi_task_range_end_HW_INIT");
MACHO_ALIAS(__nsi_PRE_BOOT_3_tasks_start, "___nsi_task_range_start_PRE_BOOT_3");
MACHO_ALIAS(__nsi_PRE_BOOT_3_tasks_end, "___nsi_task_range_end_PRE_BOOT_3");
MACHO_ALIAS(__nsi_FIRST_SLEEP_tasks_start, "___nsi_task_range_start_FIRST_SLEEP");
MACHO_ALIAS(__nsi_FIRST_SLEEP_tasks_end, "___nsi_task_range_end_FIRST_SLEEP");
MACHO_ALIAS(__nsi_ON_EXIT_PRE_tasks_start, "___nsi_task_range_start_ON_EXIT_PRE");
MACHO_ALIAS(__nsi_ON_EXIT_PRE_tasks_end, "___nsi_task_range_end_ON_EXIT_PRE");
MACHO_ALIAS(__nsi_ON_EXIT_POST_tasks_start, "___nsi_task_range_start_ON_EXIT_POST");
MACHO_ALIAS(__nsi_ON_EXIT_POST_tasks_end, "___nsi_task_range_end_ON_EXIT_POST");

/* Native simulator HW events */
MACHO_ALIAS(__nsi_hw_events_start, "section$start$__DATA$nsihwe");
MACHO_ALIAS(__nsi_hw_events_end, "section$end$__DATA$nsihwe");

#endif /* __APPLE__ */
