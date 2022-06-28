/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_

#include <stm32_ll_utils.h>

#if CONFIG_CLOCK_STM32_MCO1_SRC_NOCLOCK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_NOCLOCK
#elif CONFIG_CLOCK_STM32_MCO1_SRC_LSE
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_LSE
#elif CONFIG_CLOCK_STM32_MCO1_SRC_HSE
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSE
#elif CONFIG_CLOCK_STM32_MCO1_SRC_HSI
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSI
#elif CONFIG_CLOCK_STM32_MCO1_SRC_PLLCLK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLLCLK
#endif

#if CONFIG_CLOCK_STM32_MCO2_SRC_SYSCLK
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_SYSCLK
#elif CONFIG_CLOCK_STM32_MCO2_SRC_PLLI2S
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLLI2S
#elif CONFIG_CLOCK_STM32_MCO2_SRC_HSE
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_HSE
#elif CONFIG_CLOCK_STM32_MCO2_SRC_PLLCLK
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLLCLK
#endif

/* Macros to fill up multiplication and division factors values */
#define z_pllm(v) LL_RCC_PLLM_DIV_ ## v
#define pllm(v) z_pllm(v)

#define z_pllp(v) LL_RCC_PLLP_DIV_ ## v
#define pllp(v) z_pllp(v)

#define z_pllq(v) LL_RCC_PLLQ_DIV_ ## v
#define pllq(v) z_pllq(v)

#define z_pllr(v) LL_RCC_PLLR_DIV_ ## v
#define pllr(v) z_pllr(v)

#if defined(STM32_PLL_ENABLED)
void config_pll_sysclock(void);
uint32_t get_pllout_frequency(void);
uint32_t get_pllsrc_frequency(void);
#endif
void config_enable_default_clocks(void);

/* function exported to the soc power.c */
int stm32_clock_control_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_ */
