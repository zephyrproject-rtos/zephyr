/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief System/hardware module for RX SOC family
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <soc.h>
#include <bsp_api.h>

uint32_t SystemCoreClock BSP_SECTION_EARLY_INIT;
volatile uint32_t g_protect_pfswe_counter BSP_SECTION_EARLY_INIT;

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
	SystemCoreClock = BSP_MOCO_HZ;
	g_protect_pfswe_counter = 0;
	renesas_rx_register_protect_open();

	/* Initialize the system clock and peripheral clock */
	bsp_clock_init();
}
