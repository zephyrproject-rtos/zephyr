/*
 *
 * Copyright (c) 2018 Ilya Tagunov
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"

#if defined(STM32_PLL_ENABLED)

/* Macros to fill up multiplication and division factors values */
#define z_pll_mul(v) LL_RCC_PLL_MUL_ ## v
#define pll_mul(v) z_pll_mul(v)

#define z_pll_div(v) LL_RCC_PLL_DIV_ ## v
#define pll_div(v) z_pll_div(v)

/**
 * @brief Return PLL source
 */
__unused
static uint32_t get_pll_source(void)
{
	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		return LL_RCC_PLLSOURCE_HSI;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		return LL_RCC_PLLSOURCE_HSE;
	}

	__ASSERT(0, "Invalid source");
	return 0;
}

/**
 * @brief Set up pll configuration
 */
__unused
void config_pll_sysclock(void)
{
	LL_RCC_PLL_ConfigDomain_SYS(get_pll_source(),
				    pll_mul(STM32_PLL_MULTIPLIER),
				    pll_div(STM32_PLL_DIVISOR));
}

/**
 * @brief Return pllout frequency
 */
__unused
uint32_t get_pllout_frequency(void)
{
	return __LL_RCC_CALC_PLLCLK_FREQ(get_pll_source(),
					 pll_mul(STM32_PLL_MULTIPLIER),
					 pll_div(STM32_PLL_DIVISOR));
}

#endif /* defined(STM32_PLL_ENABLED) */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
#if defined(CONFIG_EXTI_STM32) || defined(CONFIG_USB_DC_STM32) || \
	(defined(CONFIG_SOC_SERIES_STM32L0X) &&			  \
	 defined(CONFIG_ENTROPY_STM32_RNG))
	/* Enable System Configuration Controller clock. */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
#endif
}
