/*
 *
 * Copyright (c) 2017 Linaro Limited.
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

/* Macros to fill up division factors values */
#define z_pllm(v) LL_RCC_PLLM_DIV_ ## v
#define pllm(v) z_pllm(v)

#define z_pllp(v) LL_RCC_PLLP_DIV_ ## v
#define pllp(v) z_pllp(v)

/**
 * @brief Set up pll configuration
 */
int config_pll_sysclock(void)
{
	uint32_t pll_source, pll_m, pll_n, pll_p;

	pll_n = STM32_PLL_N_MULTIPLIER;
	pll_m = pllm(STM32_PLL_M_DIVISOR);
	pll_p = pllp(STM32_PLL_P_DIVISOR);

	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		pll_source = LL_RCC_PLLSOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		pll_source = LL_RCC_PLLSOURCE_HSE;
	} else {
		return -ENOTSUP;
	}

	LL_RCC_PLL_ConfigDomain_SYS(pll_source, pll_m, pll_n, pll_p);

	return 0;
}
#endif /* STM32_SYSCLK_SRC_PLL */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
	/* Power Interface clock enabled by default */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
}
