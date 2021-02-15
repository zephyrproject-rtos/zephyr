/*
 * Copyright (c) 2019 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"


#ifdef CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL

/* Macros to fill up division factors values */
#define z_pllm(v) LL_RCC_PLLM_DIV_ ## v
#define pllm(v) z_pllm(v)

#define z_pllr(v) LL_RCC_PLLR_DIV_ ## v
#define pllr(v) z_pllr(v)

/**
 * @brief fill in pll configuration structure
 */
void config_pll_init(LL_UTILS_PLLInitTypeDef *pllinit)
{
	pllinit->PLLM = pllm(CONFIG_CLOCK_STM32_PLL_M_DIVISOR);
	pllinit->PLLN = CONFIG_CLOCK_STM32_PLL_N_MULTIPLIER;
	pllinit->PLLR = pllr(CONFIG_CLOCK_STM32_PLL_R_DIVISOR);

	/* set power boost mode for sys clock greater than 150MHz */
	if (sys_clock_hw_cycles_per_sec() >= MHZ(150)) {
		LL_PWR_EnableRange1BoostMode();
	}
}
#endif /* CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
	/* Enable the power interface clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

#ifdef CONFIG_CLOCK_STM32_LSE
	/* LSE belongs to the back-up domain, enable access.*/

	/* Set the DBP bit in the Power control register 1 (PWR_CR1) */
	LL_PWR_EnableBkUpAccess();
	while (!LL_PWR_IsEnabledBkUpAccess()) {
		/* Wait for Backup domain access */
	}

	/* Enable LSE Oscillator (32.768 kHz) */
	LL_RCC_LSE_Enable();
	while (!LL_RCC_LSE_IsReady()) {
		/* Wait for LSE ready */
	}

	LL_PWR_DisableBkUpAccess();
#endif
}

/**
 * @brief Function kept for driver genericity
 */
void LL_RCC_MSI_Disable(void)
{
	/* Do nothing */
}
