/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/ztest.h>

#if (DT_NUM_CLOCKS(DT_NODELABEL(spi5)) != 2)
#error "SPI node requires 2 clocks: bus clock and kernel clock"
#endif

ZTEST(stm32n6_devices_clocks, test_spi_clk_config)
{
	const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(spi5));
	uint32_t spi_actual_domain_clk;
	uint32_t spi_dt_clk_freq, spi_actual_clk_freq;
	int r;

	/* Test clock_on(reg_clk) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable SPI gating clock");

	zassert_true(__HAL_RCC_SPI5_IS_CLK_ENABLED(), "SPI5 gating clock should be on");

	/* Test clock_on(domain source) */
	r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				    (clock_control_subsys_t) &pclken[1],
				    NULL);
	zassert_true((r == 0), "Could not configure SPI domain clk");

	/* Test clk source */
	spi_actual_domain_clk = __HAL_RCC_GET_SPI5_SOURCE();

	switch (pclken[1].bus) {
	case STM32_SRC_PCLK2:
		zassert_equal(spi_actual_domain_clk, RCC_SPI5CLKSOURCE_PCLK2,
			      "Expected SPI src: PCLK2 (0x%x). Actual: 0x%x",
			      RCC_SPI5CLKSOURCE_PCLK2, spi_actual_domain_clk);
		break;
	case STM32_SRC_CKPER:
		zassert_equal(spi_actual_domain_clk, RCC_SPI5CLKSOURCE_CLKP,
			      "Expected SPI src: PERCLK (0x%x). Actual: 0x%x",
			      RCC_SPI5CLKSOURCE_CLKP, spi_actual_domain_clk);
		break;
	case STM32_SRC_IC9:
		zassert_equal(spi_actual_domain_clk, RCC_SPI5CLKSOURCE_IC9,
			      "Expected SPI src: IC9 (0x%x). Actual: 0x%x",
			      RCC_SPI5CLKSOURCE_IC9, spi_actual_domain_clk);
		break;
	case STM32_SRC_IC14:
		zassert_equal(spi_actual_domain_clk, RCC_SPI5CLKSOURCE_IC14,
			      "Expected SPI src: IC14 (0x%x). Actual: 0x%x",
			      RCC_SPI5CLKSOURCE_IC14, spi_actual_domain_clk);
		break;
	case STM32_SRC_MSI:
		zassert_equal(spi_actual_domain_clk, RCC_SPI5CLKSOURCE_MSI,
			      "Expected SPI src: MSI (0x%x). Actual: 0x%x",
			      RCC_SPI5CLKSOURCE_MSI, spi_actual_domain_clk);
		break;
	case STM32_SRC_HSI_DIV:
		zassert_equal(spi_actual_domain_clk, RCC_SPI5CLKSOURCE_HSI,
			      "Expected SPI src: HSI_DIV (0x%x). Actual: 0x%x",
			      RCC_SPI5CLKSOURCE_HSI, spi_actual_domain_clk);
		break;
	case STM32_SRC_HSE:
		zassert_equal(spi_actual_domain_clk, RCC_SPI5CLKSOURCE_HSE,
			      "Expected SPI src: HSE (0x%x). Actual: 0x%x",
			      RCC_SPI5CLKSOURCE_HSE, spi_actual_domain_clk);
		break;
	default:
		zassert_true(1, "Unexpected clk src (0x%x)", spi_actual_domain_clk);
		break;
	}

	/* Test get_rate(source clk) */
	r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t) &pclken[1],
				   &spi_dt_clk_freq);
	zassert_true((r == 0), "Could not get SPI clk freq");

	spi_actual_clk_freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SPI5);
	zassert_equal(spi_dt_clk_freq, spi_actual_clk_freq,
			"Expected SPI clk: %d. Actual: %d",
			spi_dt_clk_freq, spi_actual_clk_freq);

	/* Test clock_off(gating clock) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			      (clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable SPI5 reg_clk");

	zassert_true(!__HAL_RCC_SPI5_IS_CLK_ENABLED(), "SPI5 gating clock should be off");

	/* Test clock_off(domain clk) */
	/* Not supported today */
}
