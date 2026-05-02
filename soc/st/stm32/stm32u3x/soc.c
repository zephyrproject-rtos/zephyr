/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32U3 processor
 */

#include <zephyr/device.h>
#include <zephyr/cache.h>
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_system.h>
#include <zephyr/logging/log.h>

#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
#if defined(CONFIG_STM32_FLASH_PREFETCH)
	/* Enable ART Accelerator prefetch */
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
#endif

	sys_cache_instr_enable();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* The MSIS is used as system clock source after startup from reset,configured at 12 MHz. */
	SystemCoreClock = 12000000;

	/* Enable power controller bus clock for other drivers */
	LL_AHB1_GRP2_EnableClock(LL_AHB1_GRP2_PERIPH_PWR);
}
