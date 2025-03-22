/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/ztest.h>

static void i2c_set_clock(const struct stm32_pclken *clk)
{
	uint32_t dev_actual_clk_src, dev_dt_clk_freq, dev_actual_clk_freq;
	enum clock_control_status status;

	/* Test clock_on(domain_clk) */
	int r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) clk, NULL);
	zassert_true((r == 0), "Could not enable I2C domain clock");

	/* Test clock source */
	dev_actual_clk_src = __HAL_RCC_GET_I2C1_SOURCE();

	switch (clk->bus) {
	case STM32_SRC_HSI_DIV:
		zassert_equal(dev_actual_clk_src, RCC_I2C1CLKSOURCE_HSI,
				"Expected I2C src: HSI_DIV (0x%lx). Actual I2C src: 0x%x",
				RCC_I2C1CLKSOURCE_HSI, dev_actual_clk_src);
		break;
	case STM32_SRC_MSI:
		zassert_equal(dev_actual_clk_src, RCC_I2C1CLKSOURCE_MSI,
				"Expected I2C src: MSI (0x%lx). Actual I2C src: 0x%x",
				RCC_I2C1CLKSOURCE_MSI, dev_actual_clk_src);
		break;
	case STM32_SRC_CKPER:
		zassert_equal(dev_actual_clk_src, RCC_I2C1CLKSOURCE_CLKP,
				"Expected I2C src: CKPER (0x%lx). Actual I2C src: 0x%x",
				RCC_I2C1CLKSOURCE_CLKP, dev_actual_clk_src);
		break;
	case STM32_SRC_IC10:
		zassert_equal(dev_actual_clk_src, RCC_I2C1CLKSOURCE_IC10,
				"Expected I2C src: IC10 (0x%lx). Actual I2C src: 0x%x",
				RCC_I2C1CLKSOURCE_IC10, dev_actual_clk_src);
		break;
	case STM32_SRC_IC15:
		zassert_equal(dev_actual_clk_src, RCC_I2C1CLKSOURCE_IC15,
				"Expected I2C src: IC15 (0x%lx). Actual I2C src: 0x%x",
				RCC_I2C1CLKSOURCE_IC15, dev_actual_clk_src);
		break;
	default:
		zassert_true(0, "Unexpected domain clk (0x%x)", dev_actual_clk_src);
		break;
	}

	/* Test status of the used clk source */
	status = clock_control_get_status(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					  (clock_control_subsys_t)clk);
	zassert_true((status == CLOCK_CONTROL_STATUS_ON), "I2C1 clk src must to be on");

	/* Test get_rate(srce clk) */
	r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t) clk,
				   &dev_dt_clk_freq);
	zassert_true((r == 0), "Could not get I2C clk srce freq");

	dev_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_I2C1);
	zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
			"Expected freq: %d Hz. Actual clk: %d Hz",
			dev_dt_clk_freq, dev_actual_clk_freq);
}

ZTEST(stm32n6_devices_clocks, test_i2c_clk_config)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(i2c1));

	uint32_t dev_dt_clk_freq, dev_actual_clk_freq;
	enum clock_control_status status;
	int r;

	status = clock_control_get_status(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					  (clock_control_subsys_t)&pclken[0]);

	zassert_true((status == CLOCK_CONTROL_STATUS_OFF),
		     "I2C Gating clock should be off initially");

	/* Test clock_on(gating clock) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable I2C gating clock");

	/* Check via HAL as well as via get_status API */
	zassert_true(__HAL_RCC_I2C1_IS_CLK_ENABLED(), "[HAL] I2C1 gating clock should be on");
	status = clock_control_get_status(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					  (clock_control_subsys_t)&pclken[0]);
	zassert_true((status == CLOCK_CONTROL_STATUS_ON),
		     "[Zephyr] I2C1 gating clock should be on");

	if (DT_NUM_CLOCKS(DT_NODELABEL(i2c1)) > 1) {
		if (DT_NUM_CLOCKS(DT_NODELABEL(i2c1)) > 2) {
			/* set a dummy clock first, to check if the register is set correctly even
			 * if not in reset state
			 */
			i2c_set_clock(&pclken[2]);
		}
		i2c_set_clock(&pclken[1]);
	} else {
		zassert_true((DT_NUM_CLOCKS(DT_NODELABEL(i2c1)) == 1), "test config issue");
		/* No domain clock available, get rate from gating clock */

		/* Test get_rate */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					   (clock_control_subsys_t) &pclken[0],
					   &dev_dt_clk_freq);
		zassert_true((r == 0), "Could not get I2C clk freq");

		dev_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_I2C1);
		zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
				"Expected freq: %d Hz. Actual freq: %d Hz",
				dev_dt_clk_freq, dev_actual_clk_freq);
	}

	/* Test clock_off(gating clk) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			      (clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable I2C gating clk");

	zassert_true(!__HAL_RCC_I2C1_IS_CLK_ENABLED(), "I2C1 gating clk should be off");

	/* Test clock_off(srce) */
	/* Not supported today */
}
