/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RZ/A3M Group
 */

#include <zephyr/init.h>
#include <zephyr/sys/device_mmio.h>
#include "soc.h"

uint32_t SystemCoreClock;

void soc_early_init_hook(void)
{
	/*
	 * GPIO is mapped here because pinctrl_renesas_rz.c accesses the registers
	 * directly through the Renesas HAL (R_IOPORT_PinCfg()) without MMIO device API.
	 *
	 * CPG is mapped here (rather than via devicetree) because
	 * soc_early_init_hook() calls the Renesas HAL clock init (bsp_clock_init())
	 * before normal drivers.
	 */
	mm_reg_t cpg_base, gpio_base;

	device_map(&cpg_base, R_CPG_BASE, 0x1000, K_MEM_CACHE_NONE);
	device_map(&gpio_base, R_GPIO_BASE, 0x4000, K_MEM_CACHE_NONE);

	/* Configure system clocks. */
	bsp_clock_init();

	/* Initialize SystemCoreClock variable. */
	SystemCoreClockUpdate();
}
