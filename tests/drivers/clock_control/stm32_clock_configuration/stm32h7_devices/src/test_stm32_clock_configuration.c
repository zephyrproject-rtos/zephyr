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
ZTEST(stm32h7_devices_clocks, test_sysclk_freq)
{
	uint32_t soc_sys_clk_freq;

	soc_sys_clk_freq = HAL_RCC_GetSysClockFreq();

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_sys_clk_freq,
			"Expected sysclockfreq: %d. Actual sysclockfreq: %d",
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, soc_sys_clk_freq);
}

ZTEST(stm32h7_devices_clocks, test_spi_clk_config)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(spi1));
	struct stm32_pclken spi1_reg_clk_cfg = pclken[0];

	uint32_t spi1_actual_domain_clk;
	uint32_t spi1_dt_clk_freq, spi1_actual_clk_freq;
	int r;

	/* Test clock_on(reg_clk) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &spi1_reg_clk_cfg);
	zassert_true((r == 0), "Could not enable SPI reg_clk");

	zassert_true(__HAL_RCC_SPI1_IS_CLK_ENABLED(), "SPI1 reg_clk should be on");
	TC_PRINT("SPI1 reg_clk on\n");

	if (IS_ENABLED(STM32_SPI_DOMAIN_CLOCK_SUPPORT) && DT_NUM_CLOCKS(DT_NODELABEL(spi1)) > 1) {
		struct stm32_pclken spi1_domain_clk_cfg = pclken[1];

		/* Select domain_clk as device source clock */
		r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					    (clock_control_subsys_t) &spi1_domain_clk_cfg,
					    NULL);
		zassert_true((r == 0), "Could not enable SPI domain_clk");
		TC_PRINT("SPI1 domain_clk on\n");

		spi1_actual_domain_clk = __HAL_RCC_GET_SPI1_SOURCE();

		if (pclken[1].bus == STM32_SRC_PLL1_Q) {
			zassert_equal(spi1_actual_domain_clk, RCC_SPI123CLKSOURCE_PLL,
					"Expected SPI src: PLL1 Q (0x%x). Actual: 0x%x",
					RCC_SPI123CLKSOURCE_PLL, spi1_actual_domain_clk);
		} else if (pclken[1].bus == STM32_SRC_PLL2_P) {
			zassert_equal(spi1_actual_domain_clk, RCC_SPI123CLKSOURCE_PLL2,
					"Expected SPI src: PLL2 P (0x%x). Actual: 0x%x",
					RCC_SPI123CLKSOURCE_PLL2, spi1_actual_domain_clk);
		} else if (pclken[1].bus == STM32_SRC_PLL3_P) {
			zassert_equal(spi1_actual_domain_clk, RCC_SPI123CLKSOURCE_PLL3,
					"Expected SPI src: PLL3 P (0x%x). Actual: 0x%x",
					RCC_SPI123CLKSOURCE_PLL3, spi1_actual_domain_clk);
		} else if (pclken[1].bus == STM32_SRC_CKPER) {
			zassert_equal(spi1_actual_domain_clk, RCC_SPI123CLKSOURCE_CLKP,
					"Expected SPI src: PERCLK (0x%x). Actual: 0x%x",
					RCC_SPI123CLKSOURCE_CLKP, spi1_actual_domain_clk);

			/* Check perclk configuration */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(perck), okay)
			uint32_t perclk_dt_domain_clk, perclk_actual_domain_clk;

			perclk_dt_domain_clk = DT_CLOCKS_CELL_BY_IDX(DT_NODELABEL(perck), 0, bus);

			perclk_actual_domain_clk = __HAL_RCC_GET_CLKP_SOURCE();

			if (perclk_dt_domain_clk == STM32_SRC_HSI_KER) {
				zassert_equal(perclk_actual_domain_clk, RCC_CLKPSOURCE_HSI,
						"Expected PERCK src: HSI_KER (0x%x). Actual: 0x%x",
						RCC_CLKPSOURCE_HSI, perclk_actual_domain_clk);
			} else if (perclk_dt_domain_clk == STM32_SRC_CSI_KER) {
				zassert_equal(perclk_actual_domain_clk, RCC_CLKPSOURCE_CSI,
						"Expected PERCK src: CSI_KER (0x%x). Actual: 0x%x",
						RCC_CLKPSOURCE_CSI, perclk_actual_domain_clk);
			} else if (perclk_dt_domain_clk == STM32_SRC_HSE) {
				zassert_equal(perclk_actual_domain_clk, RCC_CLKPSOURCE_HSE,
						"Expected PERCK src: HSE (0x%x). Actual: 0x%x",
						RCC_CLKPSOURCE_HSE, perclk_actual_domain_clk);
			} else {
				zassert_true(0, "Unexpected PERCK domain_clk src (0x%x)",
									perclk_dt_domain_clk);
			}
#endif
		} else {
			zassert_true(0, "Unexpected domain_clk src(0x%x)", pclken[1].bus);
		}

		/* Test get_rate(domain_clk) */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &spi1_domain_clk_cfg,
					&spi1_dt_clk_freq);
		zassert_true((r == 0), "Could not get SPI clk freq");

		spi1_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI1);
		zassert_equal(spi1_dt_clk_freq, spi1_actual_clk_freq,
				"Expected SPI clk: 0x%x. Actual SPI clk: 0x%x",
				spi1_dt_clk_freq, spi1_actual_clk_freq);
	} else {
		/* No domain clock available, get rate from reg_clk */

		/* Test get_rate(reg_clk) */
		r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &spi1_reg_clk_cfg,
					&spi1_dt_clk_freq);
		zassert_true((r == 0), "Could not get SPI clk freq");

		spi1_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI1);
		zassert_equal(spi1_dt_clk_freq, spi1_actual_clk_freq,
				"Expected SPI clk: %d. Actual SPI clk: %d",
				spi1_dt_clk_freq, spi1_actual_clk_freq);
	}

	TC_PRINT("SPI1 clock freq: %d MHz\n", spi1_actual_clk_freq / (1000*1000));

	/* Test clock_off(reg_clk) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &spi1_reg_clk_cfg);
	zassert_true((r == 0), "Could not disable SPI reg_clk");

	zassert_true(!__HAL_RCC_SPI1_IS_CLK_ENABLED(), "SPI1 reg_clk should be off");
	TC_PRINT("SPI1 reg_clk off\n");

	/* Test clock_off(domain_clk) */
	/* Not supported today */
}
ZTEST_SUITE(stm32h7_devices_clocks, NULL, NULL, NULL, NULL, NULL);
