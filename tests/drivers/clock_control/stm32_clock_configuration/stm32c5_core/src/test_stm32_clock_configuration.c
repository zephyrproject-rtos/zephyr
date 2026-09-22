/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test);

ZTEST(stm32c5_clock_core_config, test_sysclk_src)
{
	hal_rcc_sysclk_src_t sys_clk_src = HAL_RCC_GetSYSCLKSource();

#if IS_ENABLED(STM32_SYSCLK_SRC_HSIS)
	zassert_equal(HAL_RCC_SYSCLK_SRC_HSIS, sys_clk_src,
			"Expected sysclk src: HSIS (0x%x). Actual: 0x%x",
			(uint32_t)HAL_RCC_SYSCLK_SRC_HSIS, sys_clk_src);
#elif IS_ENABLED(STM32_SYSCLK_SRC_HSIDIV3)
	zassert_equal(HAL_RCC_SYSCLK_SRC_HSIDIV3, sys_clk_src,
			"Expected sysclk src: HSIDIV3 (0x%x). Actual: 0x%x",
			(uint32_t)HAL_RCC_SYSCLK_SRC_HSIDIV3, sys_clk_src);
#elif IS_ENABLED(STM32_SYSCLK_SRC_HSE)
	zassert_equal(HAL_RCC_SYSCLK_SRC_HSE, sys_clk_src,
			"Expected sysclk src: HSE (0x%x). Actual: 0x%x",
			(uint32_t)HAL_RCC_SYSCLK_SRC_HSE, sys_clk_src);
#elif IS_ENABLED(STM32_SYSCLK_SRC_PSIS)
	zassert_equal(HAL_RCC_SYSCLK_SRC_PSIS, sys_clk_src,
			"Expected sysclk src: PSIS (0x%x). Actual: 0x%x",
			(uint32_t)HAL_RCC_SYSCLK_SRC_PSIS, sys_clk_src);
#else
	/* Case not expected */
	zassert_true(IS_ENABLED(STM32_SYSCLK_SRC_HSIS) ||
		     IS_ENABLED(STM32_SYSCLK_SRC_HSIDIV3) ||
		     IS_ENABLED(STM32_SYSCLK_SRC_HSE) ||
		     IS_ENABLED(STM32_SYSCLK_SRC_PSIS),
		     "Not expected. sys_clk_src: %d\n", sys_clk_src);
#endif
}

ZTEST(stm32c5_clock_core_config, test_sysclk_freq)
{
	uint32_t sysclk_freq = HAL_RCC_GetSYSCLKFreq();

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, sysclk_freq,
		      "Expected sysclk_freq: %d. Actual sysclk_freq: %d",
		      CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, sysclk_freq);
}

#if IS_ENABLED(STM32_HSE_ENABLED)
ZTEST(stm32c5_clock_core_config, test_hse_css)
{
	/* there is no function to read CSS status, so read directly from the SoC register */
	bool css_enabled = (STM32_READ_BIT(RCC->CR1, RCC_CR1_HSECSSON) == RCC_CR1_HSECSSON);

	if (IS_ENABLED(STM32_HSE_CSS)) {
		zassert_true(css_enabled, "HSE CSS is not enabled");
	} else {
		zassert_false(css_enabled, "HSE CSS unexpectedly enabled");
	}
}
#endif /* STM32_HSE_ENABLED */

#if IS_ENABLED(STM32_LSE_ENABLED)
ZTEST(stm32c5_clock_core_config, test_lse_css)
{
	bool css_enabled = (READ_BIT(RCC->RTCCR, RCC_RTCCR_LSECSSON) == 1U);

	if (IS_ENABLED(STM32_LSE_CSS)) {
		zassert_true(css_enabled, "LSE CSS is not enabled");
	} else {
		zassert_false(css_enabled, "LSE CSS unexpectedly enabled");
	}
}
#endif /* STM32_LSE_ENABLED */

ZTEST_SUITE(stm32c5_clock_core_config, NULL, NULL, NULL, NULL, NULL);
