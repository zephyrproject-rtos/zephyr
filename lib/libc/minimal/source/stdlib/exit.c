/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>

FUNC_NORETURN void _exit(int status)
{
	printk("exit\n");
	while (1) {
		Z_SPIN_DELAY(100);
	}
}
