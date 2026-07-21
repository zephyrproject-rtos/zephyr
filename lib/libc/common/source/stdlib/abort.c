/*
 * Copyright (c) 2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>

FUNC_NORETURN void abort(void)
{
	printk("abort()\n");
	k_panic();
	CODE_UNREACHABLE;
}
