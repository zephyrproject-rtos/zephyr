/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F0 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stm32_ll_system.h>
#include <zephyr/linker/linker-defs.h>
#include <string.h>

#include <cmsis_core.h>

#if defined(CONFIG_SW_VECTOR_RELAY) || defined(CONFIG_SW_VECTOR_RELAY_CLIENT)
extern void *_vector_table_pointer;
#endif

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
 * Alternatively both switches SW_VECTOR_RELAY (for Bootloader image) and
 * SW_VECTOR_RELAY_CLIENT (for image loaded by a bootloader) can be used to
 * adds a vector table relay handler and a vector relay table, to relay
 * interrupts based on a vector table pointer.
 *
 * Replaces the default function from prep_c.c.
 *
 * @note Zephyr applications that will not be loaded by a bootloader should
 *       pretend to be a bootloader if the SRAM vector table is not needed.
 */
void relocate_vector_table(void)
{
#if defined(CONFIG_SW_VECTOR_RELAY) || defined(CONFIG_SW_VECTOR_RELAY_CLIENT)
	_vector_table_pointer = _vector_start;
#elif defined(CONFIG_SRAM_VECTOR_TABLE)
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
static int stm32f0_init(void)
{
	/* Enable ART Accelerator prefetch */
	LL_FLASH_EnablePrefetch();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 8 MHz from HSI */
	SystemCoreClock = 8000000;

	return 0;
}

SYS_INIT(stm32f0_init, PRE_KERNEL_1, 0);
