/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F1 processor
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
 * @brief This function provides minimum delay (in milliseconds) based
 *	  on variable incremented.
 * @param Delay: specifies the delay time length, in milliseconds.
 * @return None
 */
void HAL_Delay(__IO uint32_t Delay)
{
	k_sleep(Delay);
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32f1_init(struct device *arg)
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
	/* At reset, system core clock is set to 8 MHz from HSI */
	SystemCoreClock = 8000000;

	return 0;
}

SYS_INIT(stm32f1_init, PRE_KERNEL_1, 0);
