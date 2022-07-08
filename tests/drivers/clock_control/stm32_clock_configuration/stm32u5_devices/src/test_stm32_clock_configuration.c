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
LOG_MODULE_REGISTER(test);

#define DT_DRV_COMPAT st_stm32_spi

#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_SPI_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_SPI_DOMAIN_CLOCK_SUPPORT 0
#endif

#define DT_NO_CLOCK 0xFFFFU

/* Not device related, but keep it to ensure core clock config is correct */
ZTEST(stm32u5_devices_clocks, test_sysclk_freq)
{
	uint32_t soc_sys_clk_freq;

	soc_sys_clk_freq = HAL_RCC_GetSysClockFreq();

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_sys_clk_freq,
			"Expected sysclockfreq: %d. Actual sysclockfreq: %d",
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_sys_clk_freq);
}

ZTEST(stm32u5_devices_clocks, test_spi_clk_config)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(spi1));

	uint32_t spi1_actual_domain_clk;
	uint32_t spi1_dt_clk_freq, spi1_actual_clk_freq;
	int r;

	/* Test clock_on(reg_clk) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable SPI gating clock");

	zassert_true(__HAL_RCC_SPI1_IS_CLK_ENABLED(), "SPI1 gating clock should be on");
	TC_PRINT("SPI1 gating clock on\n");

	if (IS_ENABLED(STM32_SPI_DOMAIN_CLOCK_SUPPORT) && DT_NUM_CLOCKS(DT_NODELABEL(spi1)) > 1) {
		/* Test clock_on(domain source) */
		r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					    (clock_control_subsys_t) &pclken[1],
					    NULL);
		zassert_true((r == 0), "Could not configure SPI domain clk");
		TC_PRINT("SPI1 domain clk configured\n");

		/* Test clk source */
		spi1_actual_domain_clk = __HAL_RCC_GET_SPI1_SOURCE();

		if (pclken[1].bus == STM32_SRC_HSI16) {
			zassert_equal(spi1_actual_domain_clk, RCC_SPI1CLKSOURCE_HSI,
					"Expected SPI src: HSI (%d). Actual SPI src: %d",
					RCC_SPI1CLKSOURCE_HSI, spi1_actual_domain_clk);
		} else if (pclken[1].bus == STM32_SRC_SYSCLK) {
			zassert_equal(spi1_actual_domain_clk, RCC_SPI1CLKSOURCE_SYSCLK,
					"Expected SPI src: SYSCLK (%d). Actual SPI src: %d",
					RCC_SPI1CLKSOURCE_SYSCLK, spi1_actual_domain_clk);
		} else {
			zassert_true(1, "Unexpected clk src(%d)", spi1_actual_domain_clk);
		}

		/* Test get_rate(source clk) */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &pclken[1],
					&spi1_dt_clk_freq);
		zassert_true((r == 0), "Could not get SPI clk freq");

		spi1_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI1);
		zassert_equal(spi1_dt_clk_freq, spi1_actual_clk_freq,
				"Expected SPI clk: (%d). Actual SPI clk: %d",
				spi1_dt_clk_freq, spi1_actual_clk_freq);
	} else {
		/* No domain clock available, get rate from gating clock */

		/* Test get_rate(gating clock) */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &pclken[0],
					&spi1_dt_clk_freq);
		zassert_true((r == 0), "Could not get SPI pclk freq");

		spi1_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI1);
		zassert_equal(spi1_dt_clk_freq, spi1_actual_clk_freq,
				"Expected SPI clk: (%d). Actual SPI clk: %d",
				spi1_dt_clk_freq, spi1_actual_clk_freq);
	}

	/* Test clock_off(gating clock) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable SPI reg_clk");

	zassert_true(!__HAL_RCC_SPI1_IS_CLK_ENABLED(), "SPI1 gating clock should be off");
	TC_PRINT("SPI1 gating clock off\n");

	/* Test clock_off(domain clk) */
	/* Not supported today */
}
ZTEST_SUITE(stm32u5_devices_clocks, NULL, NULL, NULL, NULL, NULL);
