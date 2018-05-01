/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F7 processor
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <arch/cpu.h>
#include <cortex_m/exc.h>

/**
 * @brief This function configures the source of stm32cube time base.
 *        Cube HAL expects a 1ms tick which matches with k_uptime_get_32.
 *        Tick interrupt priority is not used
 * @return HAL status
 */
uint32_t HAL_GetTick(void)
{
	return k_uptime_get_32();
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int st_stm32f7_init(struct device *arg)
{
	u32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	_ClearFaults();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 25 MHz from HSI */
	SystemCoreClock = 25000000;

	return 0;
}

SYS_INIT(st_stm32f7_init, PRE_KERNEL_1, 0);
