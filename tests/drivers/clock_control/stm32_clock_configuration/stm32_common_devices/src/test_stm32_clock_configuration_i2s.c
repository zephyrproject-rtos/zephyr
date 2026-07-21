/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(i2s2))

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2s)
#define DT_DRV_COMPAT st_stm32_i2s
#endif

ZTEST(stm32_common_devices_clocks, test_i2s_clk_config)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(i2s2));

	uint32_t dev_dt_clk_freq, dev_actual_clk_freq;
	uint32_t dev_actual_clk_src;
	int r;

	/* Test clock_on(gating clock) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable I2S gating clock");

	zassert_true(__HAL_RCC_SPI2_IS_CLK_ENABLED(), "I2S2 gating clock should be on");
	TC_PRINT("I2S2 gating clock on\n");

	zassert_true((DT_NUM_CLOCKS(DT_NODELABEL(i2s2)) > 1), "No domain clock defined in dts");

	/* Test clock_on(domain_clk) */
	r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &pclken[1],
					NULL);
	zassert_true((r == 0), "Could not enable I2S domain clock");
	TC_PRINT("I2S2 domain clock configured\n");

	/* Test clock source */
	dev_actual_clk_src = __HAL_RCC_GET_I2S_SOURCE();

	if (pclken[1].bus == STM32_SRC_PLLI2S_R) {
		zassert_equal(dev_actual_clk_src, RCC_I2SCLKSOURCE_PLLI2S,
				"Expected I2S src: PLLI2S (0x%lx). Actual I2S src: 0x%x",
				RCC_I2SCLKSOURCE_PLLI2S, dev_actual_clk_src);
	} else {
		zassert_true(0, "Unexpected domain clk (0x%x)", dev_actual_clk_src);
	}

	/* Test get_rate(srce clk) */
	r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[1],
				&dev_dt_clk_freq);
	zassert_true((r == 0), "Could not get I2S clk srce freq");

	dev_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_I2S);
	zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
			"Expected freq: %d Hz. Actual clk: %d Hz",
			dev_dt_clk_freq, dev_actual_clk_freq);

	TC_PRINT("I2S2 clock source rate: %d Hz\n", dev_dt_clk_freq);

	/* Test clock_off(gating clk) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable I2S gating clk");

	zassert_true(!__HAL_RCC_SPI2_IS_CLK_ENABLED(), "I2S2 gating clk should be off");
	TC_PRINT("I2S2 gating clk off\n");
}
#endif
