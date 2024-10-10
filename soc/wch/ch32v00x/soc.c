/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <ch32_soc.h>
#include <soc.h>

static int soc_ch32v003_init(void)
{
#if defined(CONFIG_WCH_CH32V00X_PLL)
	/* Disable the PLL before potentially changing the input clocks. */
	RCC->CTLR &= ~RCC_PLLON;
#endif
#if defined(CONFIG_WCH_CH32V00X_LSI)
	RCC->RSTSCKR |= RCC_LSION;
	while ((RCC->RSTSCKR & RCC_LSIRDY) == 0) {
	}
#endif
#if defined(CONFIG_WCH_CH32V00X_HSI)
	RCC->CTLR |= RCC_HSION;
	while ((RCC->CTLR & RCC_HSIRDY) == 0) {
	}
#endif
#if defined(CONFIG_WCH_CH32V00X_HSE)
	RCC->CTLR |= RCC_HSEON;
	while ((RCC->CTLR & RCC_HSERDY) == 0) {
	}
#endif
#if defined(CONFIG_WCH_CH32V00X_HSE_AS_PLLSRC)
	RCC->CFGR0 &= ~RCC_PLLSRC;
#elif defined(CONFIG_WCH_CH32V00X_HSI_AS_PLLSRC)
	RCC->CFGR0 &= ~RCC_PLLSRC;
#endif
#if defined(CONFIG_WCH_CH32V00X_PLL)
	RCC->CTLR |= RCC_PLLON;
	while ((RCC->CTLR & RCC_PLLRDY) == 0) {
	}
#endif
#if defined(CONFIG_WCH_CH32V00X_HSI_AS_SYSCLK)
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_HSI;
#elif defined(CONFIG_WCH_CH32V00X_HSE_AS_SYSCLK)
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_HSE;
#elif defined(CONFIG_WCH_CH32V00X_PLL_AS_SYSCLK)
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_PLL;
#endif
	RCC->CTLR |= RCC_CSSON;

	/* Clear the interrupt flags. */
	RCC->INTR = RCC_CSSC | RCC_PLLRDYC | RCC_HSERDYC | RCC_LSIRDYC;
	/* Set the Flash to 0 wait state */
	FLASH->ACTLR = (FLASH->ACTLR & ~FLASH_ACTLR_LATENCY) | FLASH_ACTLR_LATENCY_1;
	/* HCLK = SYSCLK = APB1 */
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_HPRE) | RCC_HPRE_DIV1;

	return 0;
}

SYS_INIT(soc_ch32v003_init, PRE_KERNEL_1, 0);
