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
#include <zephyr/cache.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include "soc.h"
#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

#define PWR_NODE DT_INST(0, st_stm32_pwr)

/* Helper to simplify following #if chain */
#define SELECTED_PSU(_x) DT_ENUM_HAS_VALUE(PWR_NODE, power_supply, _x)

#if SELECTED_PSU(ldo)
#define SELECTED_POWER_SUPPLY LL_PWR_LDO_SUPPLY
#elif SELECTED_PSU(smps)
#define SELECTED_POWER_SUPPLY LL_PWR_SMPS_SUPPLY
#endif

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void stm32wba_init(void)
{
	sys_cache_instr_enable();
#ifdef CONFIG_STM32_FLASH_PREFETCH
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
#endif

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	SystemCoreClock = 16000000;

	/* Enable PWR */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_PWR);

	LL_PWR_SetRegulatorSupply(SELECTED_POWER_SUPPLY);
}

void soc_early_init_hook(void)
{
	stm32wba_init();
#if CONFIG_PM
	stm32_power_init();
#endif
}
