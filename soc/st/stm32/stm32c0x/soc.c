/*
 * Copyright (c) 2023 Benjamin Björnsson <benjamin.bjornsson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32C0 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <string.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_system.h>

#include <cmsis_core.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	/* Enable ART Accelerator I-cache and prefetch */
	LL_FLASH_EnableInstCache();
	LL_FLASH_EnablePrefetch();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 48 MHz from HSI */
	SystemCoreClock = 48000000;

	/* Enable PWR clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
}
