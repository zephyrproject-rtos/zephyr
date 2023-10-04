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
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_icache.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/arch/arm/aarch32/nmi.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32u5_init(void)
{
	uint32_t key;


	key = irq_lock();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Enable instruction cache in 1-way (direct mapped cache) */
	LL_ICACHE_SetMode(LL_ICACHE_1WAY);
	LL_ICACHE_Enable();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 4 MHz from MSIS */
	SystemCoreClock = 4000000;

	/* Enable PWR */
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_PWR);

	/* Disable USB Type-C dead battery pull-down behavior */
	LL_PWR_DisableUCPDDeadBattery();

	/* Power Configuration */
#if defined(CONFIG_POWER_SUPPLY_DIRECT_SMPS)
	LL_PWR_SetRegulatorSupply(LL_PWR_SMPS_SUPPLY);
#elif defined(CONFIG_POWER_SUPPLY_LDO)
	LL_PWR_SetRegulatorSupply(LL_PWR_LDO_SUPPLY);
#else
#error "Unsupported power configuration"
#endif

	return 0;
}

SYS_INIT(stm32u5_init, PRE_KERNEL_1, 0);
