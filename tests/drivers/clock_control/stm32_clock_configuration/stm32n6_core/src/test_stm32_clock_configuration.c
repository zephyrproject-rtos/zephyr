/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/ztest.h>

ZTEST(stm32n6_clock_core_config, test_cpuclk_freq)
{
	uint32_t cpuclk_freq = HAL_RCC_GetCpuClockFreq();

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, cpuclk_freq,
		      "Expected cpuclk_freq: %d. Actual cupclk_freq: %d",
		      CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, cpuclk_freq);
}

ZTEST(stm32n6_clock_core_config, test_cpuclk_src)
{
	int cpu_clk_src = __HAL_RCC_GET_CPUCLK_SOURCE();

#if IS_ENABLED(STM32_CPUCLK_SRC_HSI)
	zassert_equal(RCC_CPUCLKSOURCE_STATUS_HSI, cpu_clk_src,
		      "Expected sysclk src: HSI (0x%x). Actual: 0x%x",
		      RCC_CPUCLKSOURCE_STATUS_HSI, cpu_clk_src);
#elif IS_ENABLED(STM32_CPUCLK_SRC_MSI)
	zassert_equal(RCC_CPUCLKSOURCE_STATUS_MSI, cpu_clk_src,
		      "Expected sysclk src: MSI (0x%x). Actual: 0x%x",
		      RCC_CPUCLKSOURCE_STATUS_MSI, cpu_clk_src);
#elif IS_ENABLED(STM32_CPUCLK_SRC_HSE)
	zassert_equal(RCC_CPUCLKSOURCE_STATUS_HSE, cpu_clk_src,
		      "Expected sysclk src: HSE (0x%x). Actual: 0x%x",
		      RCC_CPUCLKSOURCE_STATUS_HSE, cpu_clk_src);
#elif IS_ENABLED(STM32_CPUCLK_SRC_IC1)
	zassert_equal(RCC_CPUCLKSOURCE_STATUS_IC1, cpu_clk_src,
		      "Expected cpuclk src: IC1 (0x%x). Actual: 0x%x",
		      RCC_CPUCLKSOURCE_STATUS_IC1, cpu_clk_src);
#else
	/* Case not expected */
	zassert_true(IS_ENABLED(STM32_CPUCLK_SRC_HSI) ||
		     IS_ENABLED(STM32_CPUCLK_SRC_MSI) ||
		     IS_ENABLED(STM32_CPUCLK_SRC_HSE) ||
		     IS_ENABLED(STM32_CPUCLK_SRC_IC1),
		      "Not expected. cpu_clk_src: %d\n", cpu_clk_src);
#endif
}

ZTEST(stm32n6_clock_core_config, test_sysclk_src)
{
	int sys_clk_src = __HAL_RCC_GET_SYSCLK_SOURCE();

#if IS_ENABLED(STM32_SYSCLK_SRC_HSI)
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_HSI, sys_clk_src,
			"Expected sysclk src: HSI (0x%x). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_HSI, sys_clk_src);
#elif IS_ENABLED(STM32_SYSCLK_SRC_MSI)
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_MSI, sys_clk_src,
			"Expected sysclk src: MSI (0x%x). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_MSI, sys_clk_src);
#elif IS_ENABLED(STM32_SYSCLK_SRC_HSE)
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_HSE, sys_clk_src,
			"Expected sysclk src: HSE (0x%x). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_HSE, sys_clk_src);
#elif IS_ENABLED(STM32_SYSCLK_SRC_IC2)
	zassert_equal(RCC_SYSCLKSOURCE_STATUS_IC2_IC6_IC11, sys_clk_src,
			"Expected sysclk src: IC2 (0x%x). Actual: 0x%x",
			RCC_SYSCLKSOURCE_STATUS_IC2_IC6_IC11, sys_clk_src);
#else
	/* Case not expected */
	zassert_true(IS_ENABLED(STM32_SYSCLK_SRC_HSI) ||
		     IS_ENABLED(STM32_SYSCLK_SRC_MSI) ||
		     IS_ENABLED(STM32_SYSCLK_SRC_HSE) ||
		     IS_ENABLED(STM32_SYSCLK_SRC_IC2)),
		     "Not expected. sys_clk_src: %d\n", sys_clk_src);
#endif
}

#if IS_ENABLED(STM32_PLL_ENABLED)
ZTEST(stm32n6_clock_core_config, test_pll_src)
{
	uint32_t pll_src = __HAL_RCC_GET_PLL1_OSCSOURCE();

#if STM32_PLL_SRC_HSI
	zassert_equal(RCC_PLLSOURCE_HSI, pll_src,
			"Expected PLL src: HSI. Actual PLL src: %d",
			pll_src);
#elif STM32_PLL_SRC_MSI
	zassert_equal(RCC_PLLSOURCE_MSI, pll_src,
			"Expected PLL src: MSI. Actual PLL src: %d",
			pll_src);
#elif STM32_PLL_SRC_HSE
	zassert_equal(RCC_PLLSOURCE_HSE, pll_src,
			"Expected PLL src: HSE. Actual PLL src: %d",
			pll_src);
#else
	zassert_equal(RCC_PLLSOURCE_NONE, pll_src,
			"Expected PLL src: None. Actual PLL src: %d",
			pll_src);
#endif
}
#endif /* STM32_PLL_ENABLED */

#if IS_ENABLED(STM32_HSE_ENABLED)
ZTEST(stm32n6_clock_core_config, test_hse_css)
{
	/* there is no function to read CSS status, so read directly from the SoC register */
	bool css_enabled = (READ_BIT(RCC->HSECFGR, RCC_HSECFGR_HSECSSON) == 1U);

	if (IS_ENABLED(STM32_HSE_CSS)) {
		zassert_true(css_enabled, "HSE CSS is not enabled");
	} else {
		zassert_false(css_enabled, "HSE CSS unexpectedly enabled");
	}
}
#endif /* STM32_HSE_ENABLED */

#if IS_ENABLED(STM32_LSE_ENABLED)
ZTEST(stm32n6_clock_core_config, test_lse_css)
{
	/* there is no function to read CSS status, so read directly from the SoC register */
	bool css_enabled = (READ_BIT(RCC->LSECFGR, RCC_LSECFGR_LSECSSON) == 1U);

	if (IS_ENABLED(STM32_LSE_CSS)) {
		zassert_true(css_enabled, "LSE CSS is not enabled");
	} else {
		zassert_false(css_enabled, "LSE CSS unexpectedly enabled");
	}
}
#endif /* STM32_LSE_ENABLED */

#if IS_ENABLED(STM32_CKPER_ENABLED)
ZTEST(stm32n6_clock_core_config, test_perclk_config)
{
	uint32_t perclk_dt_domain_clk, perclk_actual_domain_clk;

	perclk_dt_domain_clk = DT_CLOCKS_CELL_BY_IDX(DT_NODELABEL(perck), 0, bus);
	perclk_actual_domain_clk = __HAL_RCC_GET_CLKP_SOURCE();

	switch (perclk_dt_domain_clk) {
	case STM32_SRC_HSI:
		zassert_equal(perclk_actual_domain_clk, RCC_CLKPCLKSOURCE_HSI,
			"Expected PERCK src: HSI (0x%x). Actual: 0x%x",
			RCC_CLKPCLKSOURCE_HSI, perclk_actual_domain_clk);
		break;
	case STM32_SRC_MSI:
		zassert_equal(perclk_actual_domain_clk, RCC_CLKPCLKSOURCE_MSI,
			"Expected PERCK src: MSI (0x%x). Actual: 0x%x",
			RCC_CLKPCLKSOURCE_MSI, perclk_actual_domain_clk);
		break;
	case STM32_SRC_HSE:
		zassert_equal(perclk_actual_domain_clk, RCC_CLKPCLKSOURCE_HSE,
			"Expected PERCK src: HSE (0x%x). Actual: 0x%x",
			RCC_CLKPCLKSOURCE_HSE, perclk_actual_domain_clk);
		break;
	case STM32_SRC_IC19:
		zassert_equal(perclk_actual_domain_clk, RCC_CLKPCLKSOURCE_IC19,
			"Expected PERCK src: IC19 (0x%x). Actual: 0x%x",
			RCC_CLKPCLKSOURCE_IC19, perclk_actual_domain_clk);
		break;
	case STM32_SRC_IC5:
		zassert_equal(perclk_actual_domain_clk, RCC_CLKPCLKSOURCE_IC5,
			"Expected PERCK src: IC5 (0x%x). Actual: 0x%x",
			RCC_CLKPCLKSOURCE_IC5, perclk_actual_domain_clk);
		break;
	case STM32_SRC_IC10:
		zassert_equal(perclk_actual_domain_clk, RCC_CLKPCLKSOURCE_IC10,
			"Expected PERCK src: IC10 (0x%x). Actual: 0x%x",
			RCC_CLKPCLKSOURCE_IC10, perclk_actual_domain_clk);
		break;
	case STM32_SRC_IC15:
		zassert_equal(perclk_actual_domain_clk, RCC_CLKPCLKSOURCE_IC15,
			"Expected PERCK src: IC15 (0x%x). Actual: 0x%x",
			RCC_CLKPCLKSOURCE_IC15, perclk_actual_domain_clk);
		break;
	case STM32_SRC_IC20:
		zassert_equal(perclk_actual_domain_clk, RCC_CLKPCLKSOURCE_IC20,
			"Expected PERCK src: IC20 (0x%x). Actual: 0x%x",
			RCC_CLKPCLKSOURCE_IC20, perclk_actual_domain_clk);
		break;
	default:
		zassert_true(0, "Unexpected PERCK domain_clk src (0x%x)",
			perclk_dt_domain_clk);
		break;
	}
}
#endif /* STM32_CKPER_ENABLED */

ZTEST_SUITE(stm32n6_clock_core_config, NULL, NULL, NULL, NULL, NULL);
