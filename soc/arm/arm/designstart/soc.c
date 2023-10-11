/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/arch/arm/aarch32/nmi.h>

static int arm_designstart_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/*
	 * Install default handler that simply resets the CPU if
	 * configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	return 0;
}

SYS_INIT(arm_designstart_init, PRE_KERNEL_1, 0);
