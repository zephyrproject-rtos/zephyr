/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F1 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <stm32_ll_system.h>

#include <cmsis_core.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
#ifdef FLASH_ACR_PRFTBE
	/* Enable ART Accelerator prefetch */
	LL_FLASH_EnablePrefetch();
#endif

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 8 MHz from HSI */
	SystemCoreClock = 8000000;
}
