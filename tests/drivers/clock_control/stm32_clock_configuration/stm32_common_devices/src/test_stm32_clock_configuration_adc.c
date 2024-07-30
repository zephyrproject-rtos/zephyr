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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc1), okay)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT st_stm32_adc

#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_ADC_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_ADC_DOMAIN_CLOCK_SUPPORT 0
#endif

#if defined(__HAL_RCC_GET_ADC12_SOURCE)
#define PERIPHCLK_ADC		RCC_PERIPHCLK_ADC12
#define ADC_IS_CLK_ENABLED	__HAL_RCC_ADC12_IS_CLK_ENABLED
#define GET_ADC_SOURCE		__HAL_RCC_GET_ADC12_SOURCE
#define ADC_SOURCE_SYSCLK	RCC_ADC12CLKSOURCE_SYSCLK
#elif defined(__HAL_RCC_GET_ADC_SOURCE)
#define PERIPHCLK_ADC		RCC_PERIPHCLK_ADC
#define ADC_IS_CLK_ENABLED	__HAL_RCC_ADC_IS_CLK_ENABLED
#define GET_ADC_SOURCE		__HAL_RCC_GET_ADC_SOURCE
#define ADC_SOURCE_SYSCLK	RCC_ADCCLKSOURCE_SYSCLK
#else
#define PERIPHCLK_ADC		(-1)
#define ADC_IS_CLK_ENABLED	__HAL_RCC_ADC1_IS_CLK_ENABLED
#define GET_ADC_SOURCE()	(-1);
#endif

#if defined(RCC_ADC12CLKSOURCE_PLL)
#define ADC_SOURCE_PLL		RCC_ADC12CLKSOURCE_PLL
#elif defined(RCC_ADCCLKSOURCE_PLLADC)
#define ADC_SOURCE_PLL		RCC_ADCCLKSOURCE_PLLADC
#elif defined(RCC_ADCCLKSOURCE_PLL)
#define ADC_SOURCE_PLL		RCC_ADCCLKSOURCE_PLL
#else
#define ADC_SOURCE_PLL		(-1)
#endif

ZTEST(stm32_common_devices_clocks, test_adc_clk_config)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(adc1));

	uint32_t dev_dt_clk_freq, dev_actual_clk_freq;
	uint32_t dev_actual_clk_src;
	int r;
	enum clock_control_status status;

	status = clock_control_get_status(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					  (clock_control_subsys_t)&pclken[0]);
	zassert_true((status == CLOCK_CONTROL_STATUS_OFF),
		     "ADC1 gating clock should be off initially");

	/* Test clock_on(gating clock) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable ADC1 gating clock");

	/* Check via HAL as well as via get_status API */
	zassert_true(ADC_IS_CLK_ENABLED(), "[HAL] ADC1 gating clock should be on");
	status = clock_control_get_status(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					  (clock_control_subsys_t)&pclken[0]);
	zassert_true((status == CLOCK_CONTROL_STATUS_ON),
		     "[Zephyr] ADC1 gating clock should be on");
	TC_PRINT("ADC1 gating clock on\n");

	if (IS_ENABLED(STM32_ADC_DOMAIN_CLOCK_SUPPORT) && DT_NUM_CLOCKS(DT_NODELABEL(adc1)) > 1) {
		/* Test clock_on(domain_clk) */
		r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					    (clock_control_subsys_t) &pclken[1],
					    NULL);
		zassert_true((r == 0), "Could not enable ADC1 domain clock");
		TC_PRINT("ADC1 source clock configured\n");

		/* Test clock source */
		zassert_true((ADC_SOURCE_PLL != -1), "Invalid ADC_SOURCE_PLL defined for target.");
		dev_actual_clk_src = GET_ADC_SOURCE();

		switch (pclken[1].bus) {
#if defined(STM32_SRC_PLL_P)
		case STM32_SRC_PLL_P:
			zassert_equal(dev_actual_clk_src, ADC_SOURCE_PLL,
					"Expected ADC1 src: PLL (0x%lx). Actual ADC1 src: 0x%x",
					ADC_SOURCE_PLL, dev_actual_clk_src);
			break;
#endif
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

		dev_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(PERIPHCLK_ADC);
		zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
				"Expected DT freq: %d Hz. Actual freq: %d Hz",
				dev_dt_clk_freq, dev_actual_clk_freq);

		TC_PRINT("ADC1 clock source rate: %d Hz\n", dev_dt_clk_freq);
	} else {
		zassert_true((DT_NUM_CLOCKS(DT_NODELABEL(adc1)) == 1), "test config issue");
		/* No domain clock available, don't check gating clock as for adc there is no
		 * uniform way to verify via hal.
		 */
		TC_PRINT("ADC1 no domain clock defined. Skipped check\n");
	}

	/* Test clock_off(reg_clk) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable ADC1 gating clk");

	zassert_true(!ADC_IS_CLK_ENABLED(), "ADC1 gating clk should be off");
	TC_PRINT("ADC1 gating clk off\n");

	/* Test clock_off(domain clk) */
	/* Not supported today */
}
#endif
