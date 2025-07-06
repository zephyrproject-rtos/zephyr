/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F4 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <cmsis_core.h>
#include <stm32_ll_system.h>

extern void stm32_power_init(void);

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	/* Enable ART Flash I/D-cache and prefetch */
	LL_FLASH_EnablePrefetch();
	LL_FLASH_EnableInstCache();
	LL_FLASH_EnableDataCache();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	SystemCoreClock = 16000000;
#if CONFIG_PM
	stm32_power_init();
#endif
}
