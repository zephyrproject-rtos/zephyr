/*
 * Copyright (c) 2022 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(test);

/* Not device related, but keep it to ensure core clock config is correct */
static void test_sysclk_freq(void)
{
	uint32_t soc_sys_clk_freq;

	soc_sys_clk_freq = HAL_RCC_GetSysClockFreq();

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_sys_clk_freq,
			"Expected sysclockfreq: %d. Actual sysclockfreq: %d",
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_sys_clk_freq);
}

static void test_spi_clk_config(void)
{
	struct stm32_pclken spi1_clk_cfg = {
		.enr = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(spi1), reg, bits),
		.bus = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(spi1), reg, bus)
	};
	struct stm32_pclken spi1_ker_clk_cfg = {
		.enr = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(spi1), kernel, bits),
		.bus = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(spi1), kernel, bus)
	};
	uint32_t spi1_actual_clk_src, spi1_hal_clk_kernel_src, spi1_dt_clk_kernel_src;
	uint32_t spi1_dt_clk_freq, spi1_actual_clk_freq;
	int r;

	/* Test clock_on spi1_clck_cfg */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &spi1_clk_cfg);
	zassert_true((r == 0), "Could not enable SPI Clk");

	zassert_true(__HAL_RCC_SPI1_IS_CLK_ENABLED(), "SPI1 clock should be on");
	TC_PRINT("SPI1 Clock on\n");

	/* Test clock_on spi1_ker_clk_cfg */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &spi1_ker_clk_cfg);
	zassert_true((r == 0), "Could not enable SPI ker_clk");
	TC_PRINT("SPI1 ker_clk on\n");

	/* Test clk_src */
	spi1_dt_clk_kernel_src = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(spi1), kernel, bus);
	if (spi1_dt_clk_kernel_src == STM32_SRC_PLL1_Q) {
		spi1_hal_clk_kernel_src = RCC_SPI123CLKSOURCE_PLL;
	} else if (spi1_dt_clk_kernel_src == STM32_SRC_PLL3_P) {
		spi1_hal_clk_kernel_src = RCC_SPI123CLKSOURCE_PLL3;
	} else {
		zassert_true(1, "Unexpected clk src(%d)", spi1_dt_clk_kernel_src);
	}

	spi1_actual_clk_src = __HAL_RCC_GET_SPI1_SOURCE();
	zassert_equal(spi1_hal_clk_kernel_src, __HAL_RCC_GET_SPI1_SOURCE(),
			"Expected SPI src: PLLQ (%d). Actual SPI src: %d",
			spi1_hal_clk_kernel_src, __HAL_RCC_GET_SPI1_SOURCE());

	/* Test get_rate */
	r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &spi1_ker_clk_cfg,
				&spi1_dt_clk_freq);
	zassert_true((r == 0), "Could not get SPI clk freq");

	spi1_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI1);

	zassert_equal(spi1_dt_clk_freq, spi1_actual_clk_freq,
			"Expected SPI clk: (%d). Actual SPI clk: %d",
			spi1_dt_clk_freq, spi1_actual_clk_freq);

	/* Test clock_off */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &spi1_clk_cfg);
	zassert_true((r == 0), "Could not disable SPI Clk");

	zassert_true(!__HAL_RCC_SPI1_IS_CLK_ENABLED(), "SPI1 clock should be off");
	TC_PRINT("SPI1 Clock off\n");
}

void test_main(void)
{
	ztest_test_suite(test_stm32h7_devices_clocks,
		ztest_unit_test(test_sysclk_freq),
		ztest_unit_test(test_spi_clk_config)
			 );
	ztest_run_test_suite(test_stm32h7_devices_clocks);
}
