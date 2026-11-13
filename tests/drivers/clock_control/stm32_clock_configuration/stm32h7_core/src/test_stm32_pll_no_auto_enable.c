/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <soc.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#if IS_ENABLED(STM32_PLL2_NO_AUTO_ENABLE)

ZTEST(stm32_pll_no_auto_enable, test_stopped_after_boot)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	struct stm32_pclken pll2_p = {.bus = STM32_SRC_PLL2_P, .enr = NO_SEL};

	zassert_equal(LL_RCC_PLL2_IsReady(), 0U, "PLL2 running after boot");
	zassert_equal(clock_control_get_status(clk, (clock_control_subsys_t)&pll2_p),
		      CLOCK_CONTROL_STATUS_OFF, "PLL2 P output not reported off");
}

ZTEST(stm32_pll_no_auto_enable, test_requests_are_counted)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	struct stm32_pclken pll2_p = {.bus = STM32_SRC_PLL2_P, .enr = NO_SEL};

	zassert_ok(clock_control_on(clk, (clock_control_subsys_t)&pll2_p));
	zassert_equal(LL_RCC_PLL2_IsReady(), 1U, "PLL2 not locked after first request");
	zassert_equal(clock_control_get_status(clk, (clock_control_subsys_t)&pll2_p),
		      CLOCK_CONTROL_STATUS_ON, "PLL2 P output not reported on");

	zassert_ok(clock_control_on(clk, (clock_control_subsys_t)&pll2_p));
	zassert_ok(clock_control_off(clk, (clock_control_subsys_t)&pll2_p));
	zassert_equal(LL_RCC_PLL2_IsReady(), 1U, "PLL2 stopped with a request held");

	zassert_ok(clock_control_off(clk, (clock_control_subsys_t)&pll2_p));
	zassert_equal(LL_RCC_PLL2_IsReady(), 0U, "PLL2 running after last release");
	zassert_equal(clock_control_get_status(clk, (clock_control_subsys_t)&pll2_p),
		      CLOCK_CONTROL_STATUS_OFF, "PLL2 P output not reported off");
}

ZTEST(stm32_pll_no_auto_enable, test_unbalanced_off)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	struct stm32_pclken pll2_p = {.bus = STM32_SRC_PLL2_P, .enr = NO_SEL};

	zassert_equal(clock_control_off(clk, (clock_control_subsys_t)&pll2_p), -EALREADY);
	zassert_equal(LL_RCC_PLL2_IsReady(), 0U, "PLL2 running after unbalanced release");
}

ZTEST(stm32_pll_no_auto_enable, test_off_other_source)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	struct stm32_pclken pll1_q = {.bus = STM32_SRC_PLL1_Q, .enr = NO_SEL};

	zassert_equal(clock_control_off(clk, (clock_control_subsys_t)&pll1_q), -ENOTSUP);
	zassert_equal(LL_RCC_PLL1_IsReady(), 1U, "PLL1 stopped");
}

ZTEST_SUITE(stm32_pll_no_auto_enable, NULL, NULL, NULL, NULL, NULL);

#endif /* IS_ENABLED(STM32_PLL2_NO_AUTO_ENABLE) */
