/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for MAX32xxx MCUs
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <wrap_max32_sys.h>

#if defined(CONFIG_MAX32_ON_ENTER_CPU_IDLE_HOOK)
bool z_arm_on_enter_cpu_idle(void)
{
	/* Returning false prevent device goes to sleep mode */
	return false;
}
#endif

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	/* Apply device related preinit configuration */
	max32xx_system_init();
}
