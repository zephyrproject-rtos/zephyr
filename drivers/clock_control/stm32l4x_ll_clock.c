/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <soc_registers.h>
#include <clock_control.h>
#include <misc/util.h>
#include <clock_control/stm32_clock_control.h>
#include "stm32_ll_clock.h"


#ifdef CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL

/* Macros to fill up division factors values */
#define _pllm(v) LL_RCC_PLLM_DIV_ ## v
#define pllm(v) _pllm(v)

#define _pllr(v) LL_RCC_PLLR_DIV_ ## v
#define pllr(v) _pllr(v)

/**
 * @brief fill in pll configuration structure
 */
void config_pll_init(LL_UTILS_PLLInitTypeDef *pllinit)
{
	pllinit->PLLM = pllm(CONFIG_CLOCK_STM32_PLL_M_DIVISOR);
	pllinit->PLLN = CONFIG_CLOCK_STM32_PLL_N_MULTIPLIER;
	pllinit->PLLR = pllr(CONFIG_CLOCK_STM32_PLL_R_DIVISOR);
}
#endif /* CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL */
