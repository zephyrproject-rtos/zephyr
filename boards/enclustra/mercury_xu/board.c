/*
 * Copyright (c) 2020, Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>

static int mercury_xu_init(void)
{
	return 0;
}

SYS_INIT(mercury_xu_init, PRE_KERNEL_2,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
