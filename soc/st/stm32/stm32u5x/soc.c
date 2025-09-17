/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32U5 processor
 */

#include <zephyr/device.h>
#include <zephyr/cache.h>
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <zephyr/logging/log.h>

#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

extern void stm32_power_init(void);
/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	sys_cache_instr_enable();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 4 MHz from MSIS */
	SystemCoreClock = 4000000;

	/* Enable PWR */
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_PWR);

	/* For devices with USB C PD, we can disable the dead battery
	 * pull-down behaviour.
	 */
#if defined(UCPD1)
	if (IS_ENABLED(CONFIG_DT_HAS_ST_STM32_UCPD_ENABLED) ||
		!IS_ENABLED(CONFIG_USB_DEVICE_DRIVER)) {
		/* Disable USB Type-C dead battery pull-down behavior */
		LL_PWR_DisableUCPDDeadBattery();
	}
#endif

	/* Power Configuration */
#if defined(CONFIG_POWER_SUPPLY_DIRECT_SMPS)
	LL_PWR_SetRegulatorSupply(LL_PWR_SMPS_SUPPLY);
#elif defined(CONFIG_POWER_SUPPLY_LDO)
	LL_PWR_SetRegulatorSupply(LL_PWR_LDO_SUPPLY);
#else
#error "Unsupported power configuration"
#endif

#if CONFIG_PM
	stm32_power_init();
#endif
}
