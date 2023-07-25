/*
 * Copyright (c) 2021 YuLong Yao <feilongphone@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

static int gd32e10x_soc_init(void)
{
	SystemInit();

	return 0;
}

SYS_INIT(gd32e10x_soc_init, PRE_KERNEL_1, 0);
