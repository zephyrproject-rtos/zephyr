/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F4 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
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
static int st_stm32f4_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	/* Enable ART Flash cache accelerator for both instruction and data */
	LL_FLASH_EnableInstCache();
	LL_FLASH_EnableDataCache();

	key = irq_lock();

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

SYS_INIT(st_stm32f4_init, PRE_KERNEL_1, 0);
