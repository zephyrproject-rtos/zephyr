/*
 * Copyright (c) 2017, Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

static int ti_msp432p4xx_init(void)
{

	SystemInit();

	return 0;
}

SYS_INIT(ti_msp432p4xx_init, PRE_KERNEL_1, 0);
