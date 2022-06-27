/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for ti_lm3s6965 platform
 *
 * This module provides routines to initialize and support board-level hardware
 * for the ti_lm3s6965 platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#include <zephyr/arch/cpu.h>

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers and the
 * integrated 16550-compatible UART device driver.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int ti_lm3s6965_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();
	return 0;
}

SYS_INIT(ti_lm3s6965_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
