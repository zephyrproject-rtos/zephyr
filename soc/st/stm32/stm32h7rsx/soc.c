/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32H7RS CM7 processor
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>

#include <cmsis_core.h>

#define PWR_NODE DT_INST(0, st_stm32h7rs_pwr)

/* Helper to simplify following #if chain */
#define SELECTED_PSU(_x) DT_ENUM_HAS_VALUE(PWR_NODE, power_supply, _x)

#if SELECTED_PSU(ldo)
#define SELECTED_POWER_SUPPLY LL_PWR_LDO_SUPPLY
#elif SELECTED_PSU(external_source)
#define SELECTED_POWER_SUPPLY LL_PWR_EXTERNAL_SOURCE_SUPPLY
#elif SELECTED_PSU(smps_direct)
#define SELECTED_POWER_SUPPLY LL_PWR_DIRECT_SMPS_SUPPLY
#elif SELECTED_PSU(smps_ext_ldo)
#define SELECTED_POWER_SUPPLY LL_PWR_SMPS_1V8_SUPPLIES_EXT_AND_LDO
#elif SELECTED_PSU(smps_ext_bypass)
#define SELECTED_POWER_SUPPLY LL_PWR_SMPS_1V8_SUPPLIES_EXT
#endif

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 64 MHz from HSI */
	SystemCoreClock = 64000000;

	/* Power Configuration */
	LL_PWR_ConfigSupply(SELECTED_POWER_SUPPLY);
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
	while (LL_PWR_IsActiveFlag_VOSRDY() == 0) {
	}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioo), okay) || DT_NODE_HAS_STATUS(DT_NODELABEL(gpiop), okay)
	LL_PWR_EnableXSPIM1(); /* Required for powering GPIO O and P */
#endif /* gpioo || gpio p */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpion), okay)
	LL_PWR_EnableXSPIM2(); /* Required for powering GPIO N */
#endif /* gpio n */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiom), okay)
	LL_PWR_EnableUSBVoltageDetector(); /* Required for powering GPIO M */
#endif /* gpiom */
}
