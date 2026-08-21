/*
 * Copyright (c) 2020 STMicroelectronics
 * Copyright (c) 2026 Anders Frandsen <anfran@anfran.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32WL3 processor
 */

#include <zephyr/init.h>
#include <cmsis_core.h>
#include <system_stm32wl3x.h>

/**
 * CMSIS System Core Clock: global variable holding the system core clock,
 * which is the frequency supplied to the SysTick timer and processor core.
 *
 * On STM32WL3 series, after RESET, the system clock frequency is 16 MHz.
 */
uint32_t SystemCoreClock = 16000000U;

/**
 * RAM Virtual Register: special structure located at the start
 * of SRAM0; used by the ROM bootloader.
 * Data type definition comes from @ref system_stm32wl3x.h
 */
Z_GENERIC_SECTION(stm32wl3_RAM_VR)
__used RAM_VR_TypeDef RAM_VR;

void soc_early_init_hook(void)
{
	/**
	 * Save application exception vector address in RAM_VR.
	 * By now, SCB->VTOR should point to _vector_table,
	 * so use that value instead of _vector_table directly.
	 */
	RAM_VR.AppBase = SCB->VTOR;
}
