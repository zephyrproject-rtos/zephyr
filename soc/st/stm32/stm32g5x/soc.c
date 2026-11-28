/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32G5 processor
 */

#include <cmsis_core.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>


/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	sys_cache_instr_enable();

	/* Enable SBS clock for other drivers */
	HAL_RCC_SBS_EnableClock();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 64 MHz*/
	SystemCoreClock = 64000000;
}
