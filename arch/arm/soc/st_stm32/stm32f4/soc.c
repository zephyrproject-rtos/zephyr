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

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <arch/cpu.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int st_stm32f4_init(struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* Clear all faults */
	_ScbMemFaultAllFaultsReset();
	_ScbBusFaultAllFaultsReset();
	_ScbUsageFaultAllFaultsReset();

	_ScbHardFaultAllFaultsReset();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	return 0;
}

SYS_INIT(st_stm32f4_init, PRE_KERNEL_1, 0);
