/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

ZTEST(stm32_syclck_config, test_hclk_freq)
{
	uint32_t soc_hclk_freq;

	soc_hclk_freq = HAL_RCC_GetHCLKFreq();

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_hclk_freq,
			"Expected hclk_freq: %d. Actual hclk_freq: %d",
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_hclk_freq);
}

ZTEST(stm32_syclck_config, test_sysclk_src)
{
	int sys_clk_src = __HAL_RCC_GET_SYSCLK_SOURCE();

#if STM32_SYSCLK_SRC_PLL
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_PLLCLK, sys_clk_src,
			"Expected sysclk src: PLL1 (0x%x). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_PLLCLK, sys_clk_src);
#elif STM32_SYSCLK_SRC_HSE
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_HSE, sys_clk_src,
			"Expected sysclk src: HSE (0x%x). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_HSE, sys_clk_src);
#elif STM32_SYSCLK_SRC_HSI
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_HSI, sys_clk_src,
			"Expected sysclk src: HSI (0x%x). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_HSI, sys_clk_src);
#elif STM32_SYSCLK_SRC_CSI
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_CSI, sys_clk_src,
			"Expected sysclk src: MSI (0x%x). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_CSI, sys_clk_src);
#else
	/* Case not expected */
	zassert_true((STM32_SYSCLK_SRC_PLL ||
		      STM32_SYSCLK_SRC_HSE ||
		      STM32_SYSCLK_SRC_HSI ||
		      STM32_SYSCLK_SRC_CSI),
		      "Not expected. sys_clk_src: %d\n", sys_clk_src);
#endif

}

ZTEST(stm32_syclck_config, test_pll_src)
{
	uint32_t pll_src = __HAL_RCC_GET_PLL_OSCSOURCE();

#if STM32_PLL_SRC_HSE
	zassert_equal(RCC_PLLSOURCE_HSE, pll_src,
			"Expected PLL src: HSE. Actual PLL src: %d",
			pll_src);
#elif STM32_PLL_SRC_HSI
	zassert_equal(RCC_PLLSOURCE_HSI, pll_src,
			"Expected PLL src: HSI. Actual PLL src: %d",
			pll_src);
#elif STM32_PLL_SRC_CSI
	zassert_equal(RCC_PLLSOURCE_CSI, pll_src,
			"Expected PLL src: CSI. Actual PLL src: %d",
			pll_src);
#else
	zassert_equal(RCC_PLLSOURCE_NONE, pll_src,
			"Expected PLL src: None. Actual PLL src: %d",
			pll_src);
#endif

}
ZTEST_SUITE(stm32_syclck_config, NULL, NULL, NULL, NULL, NULL);
