/*
 *
 * Copyright (c) 2018 Ilya Tagunov
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"


#ifdef CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL

/* Macros to fill up multiplication and division factors values */
#define z_pll_mul(v) LL_RCC_PLL_MUL_ ## v
#define pll_mul(v) z_pll_mul(v)

#define z_pll_div(v) LL_RCC_PLL_DIV_ ## v
#define pll_div(v) z_pll_div(v)

/**
 * @brief Fill PLL configuration structure
 */
void config_pll_init(LL_UTILS_PLLInitTypeDef *pllinit)
{
	pllinit->PLLMul = pll_mul(CONFIG_CLOCK_STM32_PLL_MULTIPLIER);
	pllinit->PLLDiv = pll_div(CONFIG_CLOCK_STM32_PLL_DIVISOR);
}

#endif /* CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
#if defined(CONFIG_EXTI_STM32) || defined(CONFIG_USB_DC_STM32)
	/* Enable System Configuration Controller clock. */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
#endif
}
