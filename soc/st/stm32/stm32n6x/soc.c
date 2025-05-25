/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32N6 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_icache.h>

#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

extern char _vector_start[];
void *g_pfnVectors = (void *)_vector_start;

#if defined(CONFIG_SOC_RESET_HOOK)
void soc_reset_hook(void)
{
	/* This is provided by STM32Cube HAL */
	SystemInit();
}
#endif

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
	/* Enable caches */
	sys_cache_instr_enable();
	sys_cache_data_enable();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 64 MHz from HSI */
	SystemCoreClock = 64000000;

	/* Enable PWR */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_PWR);

	/* Set the main internal Regulator output voltage for best performance */
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);

	/* Enable IOs */
	LL_PWR_EnableVddIO2();
	LL_PWR_EnableVddIO3();
	LL_PWR_EnableVddIO4();
	LL_PWR_EnableVddIO5();

	/* Set Vdd IO2 and IO3 to 1.8V */
	LL_PWR_SetVddIO2VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
	LL_PWR_SetVddIO3VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
}
