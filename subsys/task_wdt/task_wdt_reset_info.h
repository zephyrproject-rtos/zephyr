/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_TASK_WDT_TASK_WDT_RESET_INFO_H_
#define ZEPHYR_SUBSYS_TASK_WDT_TASK_WDT_RESET_INFO_H_

#include <zephyr/task_wdt/task_wdt.h>

/* Store a record for a channel fault, called before the system is reset */
void task_wdt_reset_info_save(const struct task_wdt_fault *fault);

/* Drop the record stored by task_wdt_reset_info_save() if no reset happened */
void task_wdt_reset_info_cancel(void);

#endif /* ZEPHYR_SUBSYS_TASK_WDT_TASK_WDT_RESET_INFO_H_ */
