/*
 * Copyright (c) 2021, pipei.sandor@gmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

static int ti_msp432e4xx_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	SystemInit();

	return 0;
}

SYS_INIT(ti_msp432e4xx_init, PRE_KERNEL_1, 0);
