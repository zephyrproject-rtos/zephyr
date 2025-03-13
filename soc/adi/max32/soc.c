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

#ifdef CONFIG_MAX32_SECONDARY_RV32
#include <fcr_regs.h>
#endif

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

#ifdef CONFIG_MAX32_SECONDARY_RV32
	MXC_FCR->urvbootaddr = CONFIG_MAX32_SECONDARY_RV32_BOOT_ADDRESS;
	MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_CPU1);
	MXC_GCR->rst1 |= MXC_F_GCR_RST1_CPU1;
#endif /* CONFIG_MAX32_SECONDARY_RV32 */
}
