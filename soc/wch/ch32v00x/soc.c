/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <ch32v003fun.h>
#include <soc.h>

#define PLL_CSS_HSE_MASK ((uint32_t)0xFEF6FFFF)
#define HSEBYP_MASK      ((uint32_t)0xFFFBFFFF)
#define PLLSRC_MASK      ((uint32_t)0xFFFEFFFF)

static void soc_ch32v003_sysclock_hsi(void)
{
	/* Flash 0 wait state */
	FLASH->ACTLR = (FLASH->ACTLR & ~FLASH_ACTLR_LATENCY) | FLASH_ACTLR_LATENCY_1;

	/* HCLK = SYSCLK = APB1 */
	RCC->CFGR0 |= RCC_HPRE_DIV1;
	/* PLLCLK = HSI * 2 */
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_PLLSRC) | RCC_PLLSRC_HSI_Mul2;

	/* Enable PLL */
	RCC->CTLR |= RCC_PLLON;
	while ((RCC->CTLR & RCC_PLLRDY) == 0) {
	}

	/* Select PLL as system clock source */
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_PLL;
	while ((RCC->CFGR0 & RCC_SWS) != RCC_SWS_PLL) {
	}
}

static int soc_ch32v003_init(void)
{
	RCC->CTLR |= RCC_HSION;
	/* PLLSRC = 0, which is the HSI */
	RCC->CFGR0 = RCC_MCO_NOCLOCK & ~RCC_PLLSRC;
	/* Turn off PLLON, CSSON, HSEON */
	RCC->CTLR &= PLL_CSS_HSE_MASK;
	/* Turn off HSEBYP */
	RCC->CTLR &= HSEBYP_MASK;
	/* Turn off PLLSRC again? */
	RCC->CFGR0 &= PLLSRC_MASK;
	/* Write to INTR to clear the CSSC, PLLRDYC, HSERDYC, and LSIRDYC flags */
	RCC->INTR = 0x009F0000;

	soc_ch32v003_sysclock_hsi();

	return 0;
}

SYS_INIT(soc_ch32v003_init, PRE_KERNEL_1, 0);
