/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32cube_hal.h>
#include <stm32_ll_rcc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

ZTEST(stm32h7p_core, test_sysclk_frequency)
{
	uint32_t sysclk_freq = HAL_RCC_GetSYSCLKFreq();

	zassert_equal(SystemCoreClock, sysclk_freq,
		      "Expected SYSCLK frequency: %u. Actual frequency: %u",
		      SystemCoreClock, sysclk_freq);
}

ZTEST(stm32h7p_core, test_sysclk_source)
{
	const uint32_t sysclk_source = LL_RCC_GetSysClkSource();
	uint32_t expected_source;

#if defined(STM32_SYSCLK_SRC_PLL)
	expected_source = LL_RCC_SYS_CLKSOURCE_STATUS_PLL1;
#elif defined(STM32_SYSCLK_SRC_HSE)
	expected_source = LL_RCC_SYS_CLKSOURCE_STATUS_HSE;
#elif defined(STM32_SYSCLK_SRC_HSI)
	expected_source = LL_RCC_SYS_CLKSOURCE_STATUS_HSI;
#elif defined(STM32_SYSCLK_SRC_CSI)
	expected_source = LL_RCC_SYS_CLKSOURCE_STATUS_CSI;
#else
#error "Unsupported SYSCLK source"
#endif

	zassert_equal(expected_source, sysclk_source,
		      "Expected SYSCLK source: 0x%x. Actual source: 0x%x",
		      expected_source, sysclk_source);
}

ZTEST(stm32h7p_core, test_pll_source)
{
	const uint32_t pll_source = LL_RCC_PLL_GetSource();
	uint32_t expected_source;

#if defined(STM32_PLL_SRC_HSI)
	expected_source = LL_RCC_PLLSOURCE_HSI;
#elif defined(STM32_PLL_SRC_HSE)
	expected_source = LL_RCC_PLLSOURCE_HSE;
#elif defined(STM32_PLL_SRC_CSI)
	expected_source = LL_RCC_PLLSOURCE_CSI;
#else
	expected_source = LL_RCC_PLLSOURCE_NO_CLK;
#endif

	zassert_equal(expected_source, pll_source,
		      "Expected PLL source: 0x%x. Actual source: 0x%x",
		      expected_source, pll_source);
}

ZTEST(stm32h7p_core, test_perck_source_freq)
{
	/* For this test, we assume DTS configures CKPER=0 (HSI source) */
	const uint32_t perck_source = LL_RCC_PERCLK_GetSource();
	const uint32_t expected_source = LL_RCC_PER_CLKSOURCE_HSI;

	zassert_equal(expected_source, perck_source,
		      "Expected PERCK source: 0x%x. Actual source: 0x%x",
		      expected_source, perck_source);

	const struct device *rcc = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct stm32_pclken perck = {
		.bus = STM32_SRC_CKPER,
		.div = 0,
		/* not used by clock_control_get_rate() */
		.enr = 0,
	};
	uint32_t rate;
	int res;

	res = clock_control_get_rate(rcc, (clock_control_subsys_t)&perck, &rate);
	zassert_equal(res, 0, "Failed to get PERCK rate (%d)", res);
	zassert_equal(MHZ(64), rate,
		      "Expected PERCK rate: %u. Actual rate: %u",
		      MHZ(64), rate);

}

ZTEST_SUITE(stm32h7p_core, NULL, NULL, NULL, NULL, NULL);
