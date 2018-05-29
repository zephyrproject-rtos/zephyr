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
#include <cortex_m/exc.h>
#include <linker/linker-defs.h>
#include <string.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32l0_init(struct device *arg)
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
	/* At reset, system core clock is set to 2.1 MHz from MSI */
	SystemCoreClock = 2097152;

	return 0;
}

SYS_INIT(stm32l0_init, PRE_KERNEL_1, 0);
