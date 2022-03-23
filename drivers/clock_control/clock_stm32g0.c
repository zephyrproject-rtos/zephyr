/*
 *
 * Copyright (c) 2019 Ilya Tagunov
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"


#if STM32_SYSCLK_SRC_PLL

/* Macros to fill up multiplication and division factors values */
#define z_pll_div(v) LL_RCC_PLLM_DIV_ ## v
#define pll_div(v) z_pll_div(v)

#define z_pllr(v) LL_RCC_PLLR_DIV_ ## v
#define pllr(v) z_pllr(v)

/**
 * @brief Set up pll configuration
 */
int config_pll_sysclock(void)
{
	uint32_t pll_source, pll_m, pll_n, pll_r;

	pll_n = STM32_PLL_N_MULTIPLIER;
	pll_m = pll_div(STM32_PLL_M_DIVISOR);
	pll_r = pllr(STM32_PLL_R_DIVISOR);

	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		pll_source = LL_RCC_PLLSOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		pll_source = LL_RCC_PLLSOURCE_HSE;
	} else {
		return -ENOTSUP;
	}

	LL_RCC_PLL_ConfigDomain_SYS(pll_source, pll_m, pll_n, pll_r);

	LL_RCC_PLL_EnableDomain_SYS();

	return 0;
}
#endif /* STM32_SYSCLK_SRC_PLL */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
	/* Enable the power interface clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
}
