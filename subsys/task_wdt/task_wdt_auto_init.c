/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/task_wdt/task_wdt.h>

static int init(void)
{
	return task_wdt_init(NULL);
}

SYS_INIT(init, POST_KERNEL, 0);
