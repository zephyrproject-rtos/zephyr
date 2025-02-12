/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for the ARM LTD Beetle SoC
 *
 * This module provides routines to initialize and support board-level hardware
 * for the ARM LTD Beetle SoC.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * Assumption:
 * MAINCLK = 24Mhz
 *
 * @return 0
 */
static int arm_beetle_init(void)
{
	/* Setup various clocks and wakeup sources */
	soc_power_init();

	return 0;
}

SYS_INIT(arm_beetle_init, PRE_KERNEL_1, 0);
