/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F0 processor
 */

#include <device.h>
#include <init.h>
#include <arch/cpu.h>
#include <cortex_m/exc.h>
#include <linker/linker-defs.h>
#include <string.h>


/**
 * @brief Relocate vector table to SRAM.
 *
 * On Cortex-M0 platforms, the Vector Base address cannot be changed.
 *
 * A Zephyr image that is run from the mcuboot bootloader must relocate the
 * vector table to SRAM to be able to replace the vectors pointing to the
 * bootloader.
 *
 * A zephyr image that is a bootloader does not have to relocate the
 * vector table.
 *
 * Replaces the default function from prep_c.c.
 *
 * @note Zephyr applications that will not be loaded by a bootloader should
 *       pretend to be a bootloader if the SRAM vector table is not needed.
 */
void relocate_vector_table(void)
{
#ifndef CONFIG_IS_BOOTLOADER
	extern char _ram_vector_start[];

	size_t vector_size = (size_t)_vector_end - (size_t)_vector_start;

	memcpy(_ram_vector_start, _vector_start, vector_size);
	LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_SRAM);
#endif
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32f0_init(struct device *arg)
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

SYS_INIT(stm32f0_init, PRE_KERNEL_1, 0);
