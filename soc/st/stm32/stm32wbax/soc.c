/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32WBA processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_icache.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include "soc.h"
#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void stm32wba_init(void)
{
	/* Enable instruction cache in 1-way (direct mapped cache) */
	LL_ICACHE_SetMode(LL_ICACHE_1WAY);
	LL_ICACHE_Enable();
#ifdef CONFIG_STM32_FLASH_PREFETCH
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
#endif

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	SystemCoreClock = 16000000;

	/* Enable PWR */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_PWR);

#if defined(CONFIG_POWER_SUPPLY_DIRECT_SMPS)
	LL_PWR_SetRegulatorSupply(LL_PWR_SMPS_SUPPLY);
#elif defined(CONFIG_POWER_SUPPLY_LDO)
	LL_PWR_SetRegulatorSupply(LL_PWR_LDO_SUPPLY);
#endif
}

void soc_early_init_hook(void)
{
	stm32wba_init();
#if CONFIG_PM
	stm32_power_init();
#endif
}
