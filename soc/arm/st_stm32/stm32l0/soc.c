/*
 * Copyright (c) 2018 Endre Karlson <endre.karlson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32L0 processor
 */

#include <device.h>
#include <init.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <linker/linker-defs.h>
#include <string.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32l0_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 2.1 MHz from MSI */
	SystemCoreClock = 2097152;

	/* Default Voltage scaling range selection (range2)
	 * doesn't allow to configure Max frequency
	 * switch to range1 to match any frequency
	 */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

	/* Enabling DBGMCU bits Sleep/Stop/Standby on STM32L0 causes Hardfault.
	 * See #37119
	 * As a workaround, force those bits to 0
	 */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_DBGMCU);
	MODIFY_REG(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP |
						   DBGMCU_CR_DBG_STOP |
						   DBGMCU_CR_DBG_STANDBY, 0U);

	return 0;
}

SYS_INIT(stm32l0_init, PRE_KERNEL_1, 0);
