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

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32g4_init(void)
{
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
	return 0;
}

SYS_INIT(stm32g4_init, PRE_KERNEL_1, 0);
