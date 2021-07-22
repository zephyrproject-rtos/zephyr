/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/__assert.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <kernel.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

static int soc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
