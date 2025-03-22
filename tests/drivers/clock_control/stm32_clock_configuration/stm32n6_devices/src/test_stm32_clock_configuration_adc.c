/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/ztest.h>

ZTEST(stm32n6_devices_clocks, test_adc_clk_config)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(adc1));

	uint32_t dev_dt_clk_freq, dev_actual_clk_freq, dev_actual_clk_src;
	enum clock_control_status status;
	int r;

	status = clock_control_get_status(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					  (clock_control_subsys_t)&pclken[0]);
	zassert_true((status == CLOCK_CONTROL_STATUS_OFF),
		     "ADC1 gating clock should be off initially");

	/* Test clock_on(gating clock) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable ADC1 gating clock");

	/* Check via HAL as well as via get_status API */
	zassert_true(__HAL_RCC_ADC12_IS_CLK_ENABLED(), "[HAL] ADC1 gating clock should be on");

	status = clock_control_get_status(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					  (clock_control_subsys_t)&pclken[0]);
	zassert_true((status == CLOCK_CONTROL_STATUS_ON),
		     "[Zephyr] ADC1 gating clock should be on");

	if (DT_NUM_CLOCKS(DT_NODELABEL(adc1)) > 1) {
		/* Test clock_on(domain_clk) */
		r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					    (clock_control_subsys_t) &pclken[1],
					    NULL);
		zassert_true((r == 0), "Could not enable ADC1 domain clock");

		/* Test clock source */
		dev_actual_clk_src = __HAL_RCC_GET_ADC_SOURCE();

		switch (pclken[1].bus) {
		case STM32_SRC_HCLK1:
			zassert_equal(dev_actual_clk_src, RCC_ADCCLKSOURCE_HCLK,
					"Expected ADC1 src: HCLK1 (0x%lx). Actual ADC1 src: 0x%x",
					RCC_ADCCLKSOURCE_HCLK, dev_actual_clk_src);
			break;
		case STM32_SRC_CKPER:
			zassert_equal(dev_actual_clk_src, RCC_ADCCLKSOURCE_CLKP,
					"Expected ADC1 src: CKPER (0x%lx). Actual ADC1 src: 0x%x",
					RCC_ADCCLKSOURCE_CLKP, dev_actual_clk_src);
			break;
		case STM32_SRC_IC7:
			zassert_equal(dev_actual_clk_src, RCC_ADCCLKSOURCE_IC7,
					"Expected ADC1 src: IC7 (0x%lx). Actual ADC1 src: 0x%x",
					RCC_ADCCLKSOURCE_IC7, dev_actual_clk_src);
			break;
		case STM32_SRC_IC8:
			zassert_equal(dev_actual_clk_src, RCC_ADCCLKSOURCE_IC8,
					"Expected ADC1 src: IC8 (0x%lx). Actual ADC1 src: 0x%x",
					RCC_ADCCLKSOURCE_IC8, dev_actual_clk_src);
			break;
		case STM32_SRC_MSI:
			zassert_equal(dev_actual_clk_src, RCC_ADCCLKSOURCE_MSI,
					"Expected ADC1 src: MSI (0x%lx). Actual ADC1 src: 0x%x",
					RCC_ADCCLKSOURCE_MSI, dev_actual_clk_src);
			break;
		case STM32_SRC_HSI_DIV:
			zassert_equal(dev_actual_clk_src, RCC_ADCCLKSOURCE_HSI,
					"Expected ADC1 src: HSI_DIV (0x%lx). Actual ADC1 src: 0x%x",
					RCC_ADCCLKSOURCE_HSI, dev_actual_clk_src);
			break;
		case STM32_SRC_TIMG:
			zassert_equal(dev_actual_clk_src, RCC_ADCCLKSOURCE_TIMG,
					"Expected ADC1 src: TIMG (0x%lx). Actual ADC1 src: 0x%x",
					RCC_ADCCLKSOURCE_TIMG, dev_actual_clk_src);
			break;
		default:
			zassert_true(0, "Unexpected src clk (%d)", dev_actual_clk_src);
		}

		/* Test status of the used clk source */
		status = clock_control_get_status(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					     (clock_control_subsys_t)&pclken[1]);
		zassert_true((status == CLOCK_CONTROL_STATUS_ON), "ADC1 clk src must to be on");

		/* Test get_rate(srce clk) */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &pclken[1],
					&dev_dt_clk_freq);
		zassert_true((r == 0), "Could not get ADC1 clk srce freq");

		dev_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_ADC);
		zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
				"Expected DT freq: %d Hz. Actual freq: %d Hz",
				dev_dt_clk_freq, dev_actual_clk_freq);

	} else {
		zassert_true((DT_NUM_CLOCKS(DT_NODELABEL(adc1)) == 1), "test config issue");
		/* No domain clock available, don't check gating clock as for adc there is no
		 * uniform way to verify via hal.
		 */
	}

	/* Test clock_off(reg_clk) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable ADC1 gating clk");

	zassert_true(!__HAL_RCC_ADC12_IS_CLK_ENABLED(), "ADC1 gating clk should be off");

	/* Test clock_off(domain clk) */
	/* Not supported today */
}
