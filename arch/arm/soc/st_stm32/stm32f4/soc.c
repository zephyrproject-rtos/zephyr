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
static int st_stm32f4_init(struct device *arg)
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

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, System core clock is set to 4MHz */
	SystemCoreClock = 4000000;
#else
	/* Update CMSIS SystemCoreClock variable (HCLK) */
	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

	return 0;
}

SYS_INIT(st_stm32f4_init, PRE_KERNEL_1, 0);
