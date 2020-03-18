/*
 * Copyright (c) 2020, Seagate
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_lpc11u6x platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_lpc11u6x platform.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <aarch32/cortex_m/exc.h>

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int nxp_lpc11u6x_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/* old interrupt lock level */
	unsigned int old_level;

	/* disable interrupts */
	old_level = irq_lock();

	/* install default handler that simply resets the CPU if configured in
	 * the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(old_level);

	return 0;
}
SYS_INIT(nxp_lpc11u6x_init, PRE_KERNEL_1, 0);
