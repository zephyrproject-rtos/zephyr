/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lptim1), okay)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT st_stm32_lptim

#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_LPTIM_OPT_CLOCK_SUPPORT 1
#else
#define STM32_LPTIM_OPT_CLOCK_SUPPORT 0
#endif

ZTEST(stm32_common_devices_clocks, test_lptim_clk_config)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(lptim1));

	uint32_t dev_dt_clk_freq, dev_actual_clk_freq;
	uint32_t dev_actual_clk_src;
	int r;

	/* Test clock_on(gating clock) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable LPTIM gating clock");

	zassert_true(__HAL_RCC_LPTIM1_IS_CLK_ENABLED(), "LPTIM1 gating clock should be on");
	TC_PRINT("LPTIM1 gating clock on\n");

	if (IS_ENABLED(STM32_LPTIM_OPT_CLOCK_SUPPORT) && DT_NUM_CLOCKS(DT_NODELABEL(lptim1)) > 1) {
		/* Test clock_on(domain_clk) */
		r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					    (clock_control_subsys_t) &pclken[1],
					    NULL);
		zassert_true((r == 0), "Could not enable LPTIM1 domain clock");
		TC_PRINT("LPTIM1 source clock configured\n");

		/* Test clock source */
		dev_actual_clk_src = __HAL_RCC_GET_LPTIM1_SOURCE();

		if (pclken[1].bus == STM32_SRC_LSE) {
			zassert_equal(dev_actual_clk_src, RCC_LPTIM1CLKSOURCE_LSE,
					"Expected LPTIM1 src: LSE (0x%lx). Actual LPTIM1 src: 0x%x",
					RCC_LPTIM1CLKSOURCE_LSE, dev_actual_clk_src);
		} else if (pclken[1].bus == STM32_SRC_LSI) {
			zassert_equal(dev_actual_clk_src, RCC_LPTIM1CLKSOURCE_LSI,
					"Expected LPTIM1 src: LSI (0x%lx). Actual LPTIM1 src: 0x%x",
					RCC_LPTIM1CLKSOURCE_LSI, dev_actual_clk_src);
		} else {
			zassert_true(0, "Unexpected domain clk (%d)", dev_actual_clk_src);
		}

		/* Test get_rate(srce clk) */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &pclken[1],
					&dev_dt_clk_freq);
		zassert_true((r == 0), "Could not get LPTIM1 clk srce freq");

		dev_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_LPTIM1);
		zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
				"Expected DT freq: %d Hz. Actual freq: %d Hz",
				dev_dt_clk_freq, dev_actual_clk_freq);

		TC_PRINT("LPTIM1 clock source rate: %d Hz\n", dev_dt_clk_freq);
	} else {
		zassert_true((DT_NUM_CLOCKS(DT_NODELABEL(lptim1)) == 1), "test config issue");
		/* No domain clock available, get rate from gating clock */

		/* Test get_rate */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &pclken[0],
					&dev_dt_clk_freq);
		zassert_true((r == 0), "Could not get LPTIM1 clk freq");

		dev_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_LPTIM1);
		zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
				"Expected DT freq: %d Hz. Actual freq: %d Hz",
				dev_dt_clk_freq, dev_actual_clk_freq);

		TC_PRINT("LPTIM1 clock source rate: %d Hz\n", dev_dt_clk_freq);
	}

	/* Test clock_off(reg_clk) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable LPTIM1 gating clk");

	zassert_true(!__HAL_RCC_LPTIM1_IS_CLK_ENABLED(), "LPTIM1 gating clk should be off");
	TC_PRINT("LPTIM1 gating clk off\n");

	/* Test clock_off(domain clk) */
	/* Not supported today */
}
#endif
