/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
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
			"Expected sysclk src: PLL1 (0x%lx). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_PLLCLK, sys_clk_src);
#elif STM32_SYSCLK_SRC_HSE
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_HSE, sys_clk_src,
			"Expected sysclk src: HSE (0x%lx). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_HSE, sys_clk_src);
#elif STM32_SYSCLK_SRC_HSI
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_HSI, sys_clk_src,
			"Expected sysclk src: HSI (0x%lx). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_HSI, sys_clk_src);
#elif STM32_SYSCLK_SRC_MSIS
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_MSI, sys_clk_src,
			"Expected sysclk src: MSI (0x%x). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_MSI, sys_clk_src);
#else
	/* Case not expected */
	zassert_true((STM32_SYSCLK_SRC_PLL ||
		      STM32_SYSCLK_SRC_HSE ||
		      STM32_SYSCLK_SRC_HSI ||
		      STM32_SYSCLK_SRC_MSIS),
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
#elif STM32_PLL_SRC_MSIS
	zassert_equal(RCC_PLLSOURCE_MSI, pll_src,
			"Expected PLL src: MSI. Actual PLL src: %d",
			pll_src);
#else
	zassert_equal(RCC_PLLSOURCE_NONE, pll_src,
			"Expected PLL src: None. Actual PLL src: %d",
			pll_src);
#endif

}

ZTEST(stm32_syclck_config, test_epod_booster)
{
#if defined(STM32_PLL_ENABLED)
	uint32_t src_freq;
	uint32_t mboost;
	uint32_t booster_freq;

	/* RM0456: the booster must be enabled and ready above 55 MHz */
	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= MHZ(55)) {
		ztest_test_skip();
		return;
	}

#if STM32_PLL_SRC_HSE
	src_freq = STM32_HSE_FREQ;
#elif STM32_PLL_SRC_HSI
	src_freq = STM32_HSI_FREQ;
#else
	src_freq = __LL_RCC_CALC_MSIS_FREQ(LL_RCC_MSIRANGESEL_RUN, LL_RCC_MSIS_GetRange());
#endif

	/* The booster clock is the PLL1 source divided by 1 or, if PLL1MBOOST = n > 0, by 2n */
	mboost = stm32_reg_read_bits(&RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1MBOOST) >>
		 RCC_PLL1CFGR_PLL1MBOOST_Pos;
	booster_freq = src_freq / ((mboost == 0U) ? 1U : (2U * mboost));

	zassert_equal(LL_PWR_IsEnabledEPODBooster(), 1U, "EPOD booster is disabled");
	zassert_equal(LL_PWR_IsActiveFlag_BOOST(), 1U, "EPOD booster is not ready");
	zassert_between_inclusive(booster_freq, MHZ(4), MHZ(16),
				  "EPOD booster clock out of range: %u Hz", booster_freq);
#else
	ztest_test_skip();
#endif
}

ZTEST_SUITE(stm32_syclck_config, NULL, NULL, NULL, NULL, NULL);
