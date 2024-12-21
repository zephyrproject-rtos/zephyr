/*
 * Copyright (c) 2017-2022 Linaro Limited.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_LL_MCO_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_LL_MCO_H_

#include <stm32_ll_utils.h>

#if CONFIG_CLOCK_STM32_MCO1_SRC_NOCLOCK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_NOCLOCK
#elif CONFIG_CLOCK_STM32_MCO1_SRC_EXT_HSE
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_EXT_HSE
#elif CONFIG_CLOCK_STM32_MCO1_SRC_LSE
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_LSE
#elif CONFIG_CLOCK_STM32_MCO1_SRC_HSE
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSE
#elif CONFIG_CLOCK_STM32_MCO1_SRC_LSI
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_LSI
#elif CONFIG_CLOCK_STM32_MCO1_SRC_MSI
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_MSI
#elif CONFIG_CLOCK_STM32_MCO1_SRC_MSIK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_MSIK
#elif CONFIG_CLOCK_STM32_MCO1_SRC_MSIS
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_MSIS
#elif CONFIG_CLOCK_STM32_MCO1_SRC_HSI
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSI
#elif CONFIG_CLOCK_STM32_MCO1_SRC_HSI16
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSI
#elif CONFIG_CLOCK_STM32_MCO1_SRC_HSI48
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_HSI48
#elif CONFIG_CLOCK_STM32_MCO1_SRC_PLLCLK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLLCLK
#elif CONFIG_CLOCK_STM32_MCO1_SRC_PLLQCLK
	#if (CONFIG_SOC_SERIES_STM32G0X || CONFIG_SOC_SERIES_STM32WLX)
		#define MCO1_SOURCE	LL_RCC_MCO1SOURCE_PLLQCLK
	#elif (CONFIG_SOC_SERIES_STM32H5X || \
		 CONFIG_SOC_SERIES_STM32H7X || CONFIG_SOC_SERIES_STM32H7RSX)
		#define MCO1_SOURCE	LL_RCC_MCO1SOURCE_PLL1QCLK
	#else
		#error "PLLQCLK is not a valid clock source on your SOC"
	#endif
#elif CONFIG_CLOCK_STM32_MCO1_SRC_PLLCLK_DIV2
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLLCLK_DIV_2
#elif CONFIG_CLOCK_STM32_MCO1_SRC_PLL2CLK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLL2CLK
#elif CONFIG_CLOCK_STM32_MCO1_SRC_PLLI2SCLK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLLI2SCLK
#elif CONFIG_CLOCK_STM32_MCO1_SRC_PLLI2SCLK_DIV2
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_PLLI2SCLK_DIV2
#elif CONFIG_CLOCK_STM32_MCO1_SRC_SYSCLK
	#define MCO1_SOURCE		LL_RCC_MCO1SOURCE_SYSCLK
#endif

#if CONFIG_CLOCK_STM32_MCO2_SRC_SYSCLK
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_SYSCLK
#elif CONFIG_CLOCK_STM32_MCO2_SRC_PLLI2S
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLLI2S
#elif CONFIG_CLOCK_STM32_MCO2_SRC_HSE
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_HSE
#elif CONFIG_CLOCK_STM32_MCO2_SRC_LSI
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_LSI
#elif CONFIG_CLOCK_STM32_MCO2_SRC_CSI
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_CSI
#elif CONFIG_CLOCK_STM32_MCO2_SRC_PLLCLK
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLLCLK
#elif CONFIG_CLOCK_STM32_MCO2_SRC_PLLPCLK
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLL1PCLK
#elif CONFIG_CLOCK_STM32_MCO2_SRC_PLL2PCLK
	#define MCO2_SOURCE		LL_RCC_MCO2SOURCE_PLL2PCLK
#endif

#define fn_mco1_prescaler(v) LL_RCC_MCO1_DIV_ ## v
#define mco1_prescaler(v) fn_mco1_prescaler(v)

#define fn_mco2_prescaler(v) LL_RCC_MCO2_DIV_ ## v
#define mco2_prescaler(v) fn_mco2_prescaler(v)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MCO configure doesn't active requested clock source,
 * so please make sure the clock source was enabled.
 */
__unused
static inline void stm32_clock_control_mco_init(void)
{
#ifndef CONFIG_CLOCK_STM32_MCO1_SRC_NOCLOCK
#ifdef CONFIG_SOC_SERIES_STM32F1X
	LL_RCC_ConfigMCO(MCO1_SOURCE);
#else
	LL_RCC_ConfigMCO(MCO1_SOURCE,
			 mco1_prescaler(CONFIG_CLOCK_STM32_MCO1_DIV));
#endif
#endif /* CONFIG_CLOCK_STM32_MCO1_SRC_NOCLOCK */

#ifndef CONFIG_CLOCK_STM32_MCO2_SRC_NOCLOCK
	LL_RCC_ConfigMCO(MCO2_SOURCE,
			 mco2_prescaler(CONFIG_CLOCK_STM32_MCO2_DIV));
#endif /* CONFIG_CLOCK_STM32_MCO2_SRC_NOCLOCK */
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_STM32_LL_MCO_H_ */
