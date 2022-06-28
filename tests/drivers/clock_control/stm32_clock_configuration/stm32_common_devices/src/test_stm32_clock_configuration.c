/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define DT_NO_CLOCK 0xFFFFU

/* Not device related, but keep it to ensure core clock config is correct */
static void test_sysclk_freq(void)
{
	uint32_t soc_sys_clk_freq;

	soc_sys_clk_freq = HAL_RCC_GetSysClockFreq();

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_sys_clk_freq,
			"Expected sysclockfreq: %d. Actual sysclockfreq: %d",
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_sys_clk_freq);
}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v1)
#define DT_DRV_COMPAT st_stm32_i2c_v1
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_i2c_v2)
#define DT_DRV_COMPAT st_stm32_i2c_v2
#endif

#if STM32_DT_INST_DEV_OPT_CLOCK_SUPPORT
#define STM32_I2C_OPT_CLOCK_SUPPORT 1
#else
#define STM32_I2C_OPT_CLOCK_SUPPORT 0
#endif

static void test_i2c_clk_config(void)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(i2c1));

	uint32_t dev_dt_clk_freq, dev_actual_clk_freq;
	uint32_t dev_actual_clk_src;
	int r;

	/* Test clock_on(gating clock) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable I2C gating clock");

	zassert_true(__HAL_RCC_I2C1_IS_CLK_ENABLED(), "I2C1 gating clock should be on");
	TC_PRINT("I2C1 gating clock on\n");

	if (IS_ENABLED(STM32_I2C_OPT_CLOCK_SUPPORT) && DT_NUM_CLOCKS(DT_NODELABEL(i2c1)) > 1) {
		/* Test clock_on(ker_clk) */
		r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					    (clock_control_subsys_t) &pclken[1],
					    NULL);
		zassert_true((r == 0), "Could not enable I2C soure clock");
		TC_PRINT("I2C1 source clock configured\n");

		/* Test clock source */
		dev_actual_clk_src = __HAL_RCC_GET_I2C1_SOURCE();

		if (pclken[1].bus == STM32_SRC_HSI) {
			zassert_equal(dev_actual_clk_src, RCC_I2C1CLKSOURCE_HSI,
					"Expected I2C src: HSI (0x%lx). Actual I2C src: 0x%x",
					RCC_I2C1CLKSOURCE_HSI, dev_actual_clk_src);
		} else if (pclken[1].bus == STM32_SRC_SYSCLK) {
			zassert_equal(dev_actual_clk_src, RCC_I2C1CLKSOURCE_SYSCLK,
					"Expected I2C src: SYSCLK (0x%lx). Actual I2C src: 0x%x",
					RCC_I2C1CLKSOURCE_SYSCLK, dev_actual_clk_src);
		} else {
			zassert_true(0, "Unexpected src clk (%d)", dev_actual_clk_src);
		}

		/* Test get_rate(srce clk) */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &pclken[1],
					&dev_dt_clk_freq);
		zassert_true((r == 0), "Could not get I2C clk srce freq");

		dev_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_I2C1);
		zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
				"Expected freq: %d Hz. Actual clk: %d Hz",
				dev_dt_clk_freq, dev_actual_clk_freq);

		TC_PRINT("I2C1 clock source rate: %d Hz\n", dev_dt_clk_freq);
	} else {
		zassert_true((DT_NUM_CLOCKS(DT_NODELABEL(i2c1)) == 1), "test config issue");
		/* No alt clock available, get rate from gating clock */

		/* Test get_rate */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &pclken[0],
					&dev_dt_clk_freq);
		zassert_true((r == 0), "Could not get I2C clk freq");

		dev_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_I2C1);
		zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
				"Expected freq: %d Hz. Actual freq: %d Hz",
				dev_dt_clk_freq, dev_actual_clk_freq);

		TC_PRINT("I2C1 clock source rate: %d Hz\n", dev_dt_clk_freq);
	}

	/* Test clock_off(gating clk) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable I2C gating clk");

	zassert_true(!__HAL_RCC_I2C1_IS_CLK_ENABLED(), "I2C1 gating clk should be off");
	TC_PRINT("I2C1 gating clk off\n");

	/* Test clock_off(srce) */
	/* Not supported today */
}
#else
static void test_i2c_clk_config(void) {}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lptim1), okay)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT st_stm32_lptim

#if STM32_DT_INST_DEV_OPT_CLOCK_SUPPORT
#define STM32_LPTIM_OPT_CLOCK_SUPPORT 1
#else
#define STM32_LPTIM_OPT_CLOCK_SUPPORT 0
#endif

static void test_lptim_clk_config(void)
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
		/* Test clock_on(ker_clk) */
		r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					    (clock_control_subsys_t) &pclken[1],
					    NULL);
		zassert_true((r == 0), "Could not enable LPTIM1 soure clock");
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
			zassert_true(0, "Unexpected src clk (%d)", dev_actual_clk_src);
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
		/* No alt clock available, get rate from gating clock */

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

	/* Test clock_off(srce) */
	/* Not supported today */
}
#else
static void test_lptim_clk_config(void) {}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc1), okay)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT st_stm32_adc

#if STM32_DT_INST_DEV_OPT_CLOCK_SUPPORT
#define STM32_ADC_OPT_CLOCK_SUPPORT 1
#else
#define STM32_ADC_OPT_CLOCK_SUPPORT 0
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

static void test_adc_clk_config(void)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(adc1));

	uint32_t dev_dt_clk_freq, dev_actual_clk_freq;
	uint32_t dev_actual_clk_src;
	int r;

	/* Test clock_on(gating clock) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable ADC1 gating clock");

	zassert_true(ADC_IS_CLK_ENABLED(), "ADC1 gating clock should be on");
	TC_PRINT("ADC1 gating clock on\n");

	if (IS_ENABLED(STM32_ADC_OPT_CLOCK_SUPPORT) && DT_NUM_CLOCKS(DT_NODELABEL(adc1)) > 1) {
		/* Test clock_on(ker_clk) */
		r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					    (clock_control_subsys_t) &pclken[1],
					    NULL);
		zassert_true((r == 0), "Could not enable ADC1 soure clock");
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
		/* No alt clock available, don't check gating clock as for adc there is no
		 * uniform way to verify via hal.
		 */
		TC_PRINT("ADC1 no alt clock defined. Skipped check\n");
	}

	/* Test clock_off(reg_clk) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable ADC1 gating clk");

	zassert_true(!ADC_IS_CLK_ENABLED(), "ADC1 gating clk should be off");
	TC_PRINT("ADC1 gating clk off\n");

	/* Test clock_off(srce) */
	/* Not supported today */
}
#else
static void test_adc_clk_config(void) {}
#endif

void test_main(void)
{
	ztest_test_suite(test_stm32_common_devices_clocks,
		ztest_unit_test(test_sysclk_freq),
		ztest_unit_test(test_i2c_clk_config),
		ztest_unit_test(test_lptim_clk_config),
		ztest_unit_test(test_adc_clk_config)
			 );
	ztest_run_test_suite(test_stm32_common_devices_clocks);
}
