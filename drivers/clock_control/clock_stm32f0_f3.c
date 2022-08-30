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
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"

#if defined(STM32_PLL_ENABLED)

/**
 * @brief Set up pll configuration
 */
__unused
void config_pll_sysclock(void)
{
	uint32_t pll_source, pll_mul, pll_div;

	/*
	 * PLL MUL
	 * 2  -> LL_RCC_PLL_MUL_2  -> 0x00000000
	 * 3  -> LL_RCC_PLL_MUL_3  -> 0x00040000
	 * 4  -> LL_RCC_PLL_MUL_4  -> 0x00080000
	 * ...
	 * 16 -> LL_RCC_PLL_MUL_16 -> 0x00380000
	 */
	pll_mul = ((STM32_PLL_MULTIPLIER - 2) << RCC_CFGR_PLLMUL_Pos);

	/*
	 * PLL PREDIV
	 * 1  -> LL_RCC_PREDIV_DIV_1  -> 0x00000000
	 * 2  -> LL_RCC_PREDIV_DIV_2  -> 0x00000001
	 * 3  -> LL_RCC_PREDIV_DIV_3  -> 0x00000002
	 * ...
	 * 16 -> LL_RCC_PREDIV_DIV_16 -> 0x0000000F
	 */
	pll_div = STM32_PLL_PREDIV - 1;

#if defined(RCC_PLLSRC_PREDIV1_SUPPORT)
	/*
	 * PREDIV1 support is a specific RCC configuration present on
	 * following SoCs: STM32F04xx, STM32F07xx, STM32F09xx,
	 * STM32F030xC, STM32F302xE, STM32F303xE and STM32F39xx
	 * cf Reference manual for more details
	 */

	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		pll_source = LL_RCC_PLLSOURCE_HSE;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		pll_source = LL_RCC_PLLSOURCE_HSI;
	} else {
		__ASSERT(0, "Invalid source");
	}

	LL_RCC_PLL_ConfigDomain_SYS(pll_source, pll_mul, pll_div);
#else
	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		pll_source = LL_RCC_PLLSOURCE_HSE | pll_div;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		pll_source = LL_RCC_PLLSOURCE_HSI_DIV_2;
	} else {
		__ASSERT(0, "Invalid source");
	}

	LL_RCC_PLL_ConfigDomain_SYS(pll_source, pll_mul);
#endif /* RCC_PLLSRC_PREDIV1_SUPPORT */
}

/**
 * @brief Return pllout frequency
 */
__unused
uint32_t get_pllout_frequency(void)
{
	uint32_t pll_input_freq, pll_mul, pll_div;

	/*
	 * PLL MUL
	 * 2  -> LL_RCC_PLL_MUL_2  -> 0x00000000
	 * 3  -> LL_RCC_PLL_MUL_3  -> 0x00040000
	 * 4  -> LL_RCC_PLL_MUL_4  -> 0x00080000
	 * ...
	 * 16 -> LL_RCC_PLL_MUL_16 -> 0x00380000
	 */
	pll_mul = ((STM32_PLL_MULTIPLIER - 2) << RCC_CFGR_PLLMUL_Pos);

	/*
	 * PLL PREDIV
	 * 1  -> LL_RCC_PREDIV_DIV_1  -> 0x00000000
	 * 2  -> LL_RCC_PREDIV_DIV_2  -> 0x00000001
	 * 3  -> LL_RCC_PREDIV_DIV_3  -> 0x00000002
	 * ...
	 * 16 -> LL_RCC_PREDIV_DIV_16 -> 0x0000000F
	 */
	pll_div = STM32_PLL_PREDIV - 1;

#if defined(RCC_PLLSRC_PREDIV1_SUPPORT)
	/*
	 * PREDIV1 support is a specific RCC configuration present on
	 * following SoCs: STM32F04xx, STM32F07xx, STM32F09xx,
	 * STM32F030xC, STM32F302xE, STM32F303xE and STM32F39xx
	 * cf Reference manual for more details
	 */

	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		pll_input_freq = STM32_HSE_FREQ;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		pll_input_freq = STM32_HSI_FREQ;
	} else {
		return 0;
	}

	return __LL_RCC_CALC_PLLCLK_FREQ(pll_input_freq, pll_mul, pll_div);
#else
	/* Configure PLL source */
	if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		pll_input_freq = STM32_HSE_FREQ;
	} else if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		pll_input_freq = STM32_HSI_FREQ / 2;
	}  else {
		return 0;
	}

	return __LL_RCC_CALC_PLLCLK_FREQ(pll_input_freq, pll_mul);
#endif /* RCC_PLLSRC_PREDIV1_SUPPORT */
}

#endif /* defined(STM32_PLL_ENABLED) */

/**
 * @brief Activate default clocks
 */
void config_enable_default_clocks(void)
{
#ifndef CONFIG_SOC_SERIES_STM32F3X
#if defined(CONFIG_EXTI_STM32) || defined(CONFIG_USB_DC_STM32)
	/* Enable System Configuration Controller clock. */
	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
#endif
#else
#if defined(CONFIG_USB_DC_STM32) && defined(SYSCFG_CFGR1_USB_IT_RMP)
	/* Enable System Configuration Controller clock. */
	/* SYSCFG is required to remap IRQ to avoid conflicts with CAN */
	/* cf §14.1.3, RM0316 */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
#endif
#endif /* !CONFIG_SOC_SERIES_STM32F3X */
}
