/*
 * Copyright (c) 2017 Linaro Limited
 *
 * Initial contents based on arch/arm/soc/ti_lm3s6965/soc.c which is:
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/cpu.h>
#include <init.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * @return 0
 */
static int arm_mps2_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/*
	 * Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	return 0;
}

SYS_INIT(arm_mps2_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
