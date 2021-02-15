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


#ifdef CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL

/* Macros to fill up multiplication and division factors values */
#define z_pll_div(v) LL_RCC_PLLM_DIV_ ## v
#define pll_div(v) z_pll_div(v)

#define z_pllr(v) LL_RCC_PLLR_DIV_ ## v
#define pllr(v) z_pllr(v)

/**
 * @brief Fill PLL configuration structure
 */
void config_pll_init(LL_UTILS_PLLInitTypeDef *pllinit)
{
	pllinit->PLLN = CONFIG_CLOCK_STM32_PLL_N_MULTIPLIER;
	pllinit->PLLM = pll_div(CONFIG_CLOCK_STM32_PLL_M_DIVISOR);
	pllinit->PLLR = pllr(CONFIG_CLOCK_STM32_PLL_R_DIVISOR);
}
#endif /* CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
	/* Enable the power interface clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
}

/**
 * @brief Function kept for driver genericity
 */
void LL_RCC_MSI_Disable(void)
{
	/* Do nothing */
}
