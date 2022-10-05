/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F7 processor
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/arch/arm/aarch32/nmi.h>
#include <zephyr/irq.h>
#include <stm32_ll_system.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int st_stm32f7_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	/* Enable ART Flash cache accelerator */
	LL_FLASH_EnableART();

	key = irq_lock();

	SCB_EnableICache();

	if (IS_ENABLED(CONFIG_DCACHE)) {
		if (!(SCB->CCR & SCB_CCR_DC_Msk)) {
			SCB_EnableDCache();
		}
	}

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	SystemCoreClock = 16000000;

	return 0;
}

SYS_INIT(st_stm32f7_init, PRE_KERNEL_1, 0);
