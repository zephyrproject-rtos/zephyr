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
#include <stm32f1xx_ll_bus.h>

#include <cmsis_core.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
#if defined(FLASH_ACR_PRFTBE) && defined(CONFIG_STM32_FLASH_PREFETCH)
	/* Enable ART Accelerator prefetch */
	LL_FLASH_EnablePrefetch();
#endif

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 8 MHz from HSI */
	SystemCoreClock = 8000000;

#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
#endif
}
