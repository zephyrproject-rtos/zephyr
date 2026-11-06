/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test);

ZTEST(stm32g5_clock_core_config, test_sysclk_src)
{
	hal_rcc_sysclk_src_t sys_clk_src = HAL_RCC_GetSYSCLKSource();

#if IS_ENABLED(STM32_SYSCLK_SRC_PLL)
	zassert_equal(HAL_RCC_SYSCLK_SRC_PLLCLK, sys_clk_src,
		      "Expected sysclk src: PLL1 (0x%x). Actual: 0x%x",
		      (uint32_t)HAL_RCC_SYSCLK_SRC_PLLCLK, sys_clk_src);
#elif IS_ENABLED(STM32_SYSCLK_SRC_HSE)
	zassert_equal(HAL_RCC_SYSCLK_SRC_HSE, sys_clk_src,
		      "Expected sysclk src: HSE (0x%x). Actual: 0x%x",
		      (uint32_t)HAL_RCC_SYSCLK_SRC_HSE, sys_clk_src);
#elif IS_ENABLED(STM32_SYSCLK_SRC_HSI)
	zassert_equal(HAL_RCC_SYSCLK_SRC_HSI, sys_clk_src,
		      "Expected sysclk src: HSI (0x%x). Actual: 0x%x",
		      (uint32_t)HAL_RCC_SYSCLK_SRC_HSI, sys_clk_src);
#else
	/* Case not expected */
	zassert_true(IS_ENABLED(STM32_SYSCLK_SRC_PLL) ||
		     IS_ENABLED(STM32_SYSCLK_SRC_HSE) ||
		     IS_ENABLED(STM32_SYSCLK_SRC_HSI),
		     "Not expected. sys_clk_src: %d\n", sys_clk_src);
#endif
}

ZTEST(stm32g5_clock_core_config, test_sysclk_freq)
{
	uint32_t sysclk_freq = HAL_RCC_GetSYSCLKFreq();
	uint32_t expected_freq = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC * STM32_AHB_PRESCALER;

	zassert_equal(expected_freq, sysclk_freq,
		      "Expected SYSCLK: %u. Actual: %u", expected_freq, sysclk_freq);
}

ZTEST(stm32g5_clock_core_config, test_ahb_prescaler)
{
	zassert_equal(CONCAT(LL_RCC_HCLK_PRESCALER_, STM32_AHB_PRESCALER),
		      LL_RCC_GetAHBPrescaler(), "Unexpected AHB prescaler");
}

ZTEST(stm32g5_clock_core_config, test_hclk_freq)
{
	uint32_t hclk_freq = HAL_RCC_GetHCLKFreq();

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, hclk_freq,
		      "Expected hclk_freq: %d. Actual hclk_freq: %d",
		      CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, hclk_freq);
}

ZTEST_SUITE(stm32g5_clock_core_config, NULL, NULL, NULL, NULL, NULL);
