/*
 * Copyright (c) 2019 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32G4 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stm32_ll_system.h>
#include <soc.h>

#include <cmsis_core.h>
#if defined(PWR_CR3_UCPD_DBDIS)
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#endif /* PWR_CR3_UCPD_DBDIS */

extern void stm32_power_init(void);

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	/* Enable ART Accelerator I/D-cache and prefetch */
	LL_FLASH_EnableInstCache();
	LL_FLASH_EnableDataCache();
	LL_FLASH_EnablePrefetch();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	SystemCoreClock = 16000000;

	/* allow reflashing board */
	LL_DBGMCU_EnableDBGSleepMode();

#if defined(PWR_CR3_UCPD_DBDIS)
	if (IS_ENABLED(CONFIG_DT_HAS_ST_STM32_UCPD_ENABLED) ||
		!IS_ENABLED(CONFIG_USB_DEVICE_DRIVER)) {
		/* Disable USB Type-C dead battery pull-down behavior */
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		LL_PWR_DisableUCPDDeadBattery();
	}

#endif /* PWR_CR3_UCPD_DBDIS */
#if CONFIG_PM
	stm32_power_init();
#endif
}
