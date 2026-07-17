/*
 * Copyright (c) 2026 Anders Frandsen <anfran@anfran.dk>
 * Copyright (c) 2020 STMicroelectronics
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

uint32_t SystemCoreClock = 16000000U;

Z_GENERIC_SECTION(stm32wl3_RAM_VR)
__used RAM_VR_TypeDef RAM_VR;

void soc_early_init_hook(void)
{
	RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_CLKSYSDIV) | RCC_CFGR_CLKSYSDIV_1;
	RCC->APB0ENR |= RCC_APB0ENR_SYSCFGEN;

	SystemCoreClock = 16000000U;
	RAM_VR.AppBase = SCB->VTOR;
}
