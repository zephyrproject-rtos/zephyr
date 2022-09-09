/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <stm32_ll_rcc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#if defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
#define CALC_HCLK_FREQ __LL_RCC_CALC_HCLK1_FREQ
#else
#define CALC_HCLK_FREQ __LL_RCC_CALC_HCLK_FREQ
#endif

ZTEST(stm32_sysclck_config, test_hclk_freq)
{
	uint32_t soc_hclk_freq;

	soc_hclk_freq = CALC_HCLK_FREQ(HAL_RCC_GetSysClockFreq(),
					LL_RCC_GetAHBPrescaler());

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_hclk_freq,
			"Expected hclck_freq: %d. Actual hclck_freq: %d",
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_hclk_freq);
}

ZTEST(stm32_sysclck_config, test_sysclk_src)
{
	int sys_clk_src = __HAL_RCC_GET_SYSCLK_SOURCE();

#if STM32_SYSCLK_SRC_PLL
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_PLLCLK, sys_clk_src,
			"Expected sysclk src: PLL. Actual sysclk src: %d",
			sys_clk_src);
#elif STM32_SYSCLK_SRC_HSE
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_HSE, sys_clk_src,
			"Expected sysclk src: HSE. Actual sysclk src: %d",
			sys_clk_src);
#elif STM32_SYSCLK_SRC_HSI
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_HSI, sys_clk_src,
			"Expected sysclk src: HSI. Actual sysclk src: %d",
			sys_clk_src);
#elif STM32_SYSCLK_SRC_MSI
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_MSI, sys_clk_src,
			"Expected sysclk src: MSI. Actual sysclk src: %d",
			sys_clk_src);
#else
	/* Case not expected */
	zassert_true((STM32_SYSCLK_SRC_PLL ||
		      STM32_SYSCLK_SRC_HSE ||
		      STM32_SYSCLK_SRC_HSI ||
		      STM32_SYSCLK_SRC_MSI),
		      "Not expected. sys_clk_src: %d\n", sys_clk_src);
#endif

}

ZTEST(stm32_sysclck_config, test_pll_src)
{
	uint32_t pll_src = __HAL_RCC_GET_PLL_OSCSOURCE();

#if STM32_PLL_SRC_HSE
	zassert_equal(RCC_PLLSOURCE_HSE, pll_src,
			"Expected PLL src: HSE (%d). Actual PLL src: %d",
			RCC_PLLSOURCE_HSE, pll_src);
#elif STM32_PLL_SRC_HSI
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	zassert_equal(RCC_PLLSOURCE_HSI_DIV2, pll_src,
			"Expected PLL src: HSI (%d). Actual PLL src: %d",
			RCC_PLLSOURCE_HSI_DIV2, pll_src);
#else
	zassert_equal(RCC_PLLSOURCE_HSI, pll_src,
			"Expected PLL src: HSI (%d). Actual PLL src: %d",
			RCC_PLLSOURCE_HSI, pll_src);
#endif /* CONFIG_SOC_SERIES_STM32F1X */
#elif STM32_PLL_SRC_MSI
	zassert_equal(RCC_PLLSOURCE_MSI, pll_src,
			"Expected PLL src: MSI (%d). Actual PLL src: %d",
			RCC_PLLSOURCE_MSI, pll_src);
#else /* --> RCC_PLLSOURCE_NONE */
#if defined(CONFIG_SOC_SERIES_STM32L0X) || defined(CONFIG_SOC_SERIES_STM32L1X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || defined(CONFIG_SOC_SERIES_STM32F7X)
#define RCC_PLLSOURCE_NONE 0
	/* check RCC_CR_PLLON bit to enable/disable the PLL, but no status function exist */
	if (READ_BIT(RCC->CR, RCC_CR_PLLON) == RCC_CR_PLLON) {
		/* should not happen : PLL must be disabled when not used */
		pll_src = 0xFFFF; /* error code */
	} else {
		pll_src = RCC_PLLSOURCE_NONE;
	}
#endif /* RCC_CR_PLLON */
	zassert_equal(RCC_PLLSOURCE_NONE, pll_src,
			"Expected PLL src: none (%d). Actual PLL src: %d",
			RCC_PLLSOURCE_NONE, pll_src);

#endif

}
ZTEST_SUITE(stm32_sysclck_config, NULL, NULL, NULL, NULL, NULL);
