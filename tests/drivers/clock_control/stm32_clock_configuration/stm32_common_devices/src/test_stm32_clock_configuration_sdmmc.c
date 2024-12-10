/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sdmmc1))

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_sdmmc)
#define DT_DRV_COMPAT st_stm32_sdmmc1
#endif

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32_clock_mux)
#warning "Missing clock 48MHz"
#endif

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f411_plli2s_clock)
#warning "Missing clock I2S PLL clock"
#endif

#include "stm32_ll_rcc.h"

ZTEST(stm32_common_devices_clocks, test_sdmmc_clk_config)
{
	static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(DT_NODELABEL(sdmmc1));

	uint32_t dev_dt_clk_freq, dev_actual_clk_freq;
	uint32_t dev_actual_clk_src;
	int r;

	/* Test clock_on(gating clock) */
	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not enable SDMMC gating clock");

	zassert_true(__HAL_RCC_SDIO_IS_CLK_ENABLED(), "SDMMC gating clock should be on");
	TC_PRINT("SDMMC gating clock on\n");

	zassert_true((DT_NUM_CLOCKS(DT_NODELABEL(sdmmc1)) > 1), "No domain clock defined in dts");

	if (pclken[1].bus == STM32_SRC_CK48) {
		/* CLK 48 is enabled through the clock-mux */
		zassert_true(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk48)), "No clock 48MHz");
		r = 0;
	} else if (pclken[1].bus == STM32_SRC_SYSCLK) {
		/* Test clock_on(domain_clk) STM32_SRC_SYSCLK */
		r = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &pclken[1],
					NULL);
	} else {
		r = -127;
	}

	zassert_true((r == 0), "Could not enable SDMMC domain clock");
	TC_PRINT("SDMMC domain clock configured\n");

	/* Test clock source */
	dev_actual_clk_src = __HAL_RCC_GET_SDIO_SOURCE();

	if (pclken[1].bus == STM32_SRC_CK48) {
		zassert_equal(dev_actual_clk_src, RCC_SDIOCLKSOURCE_CLK48,
				"Expected SDMMC src: CLK 48 (0x%lx). Actual src: 0x%x",
				RCC_SDIOCLKSOURCE_CLK48, dev_actual_clk_src);
	} else if (pclken[1].bus == STM32_SRC_SYSCLK) {
		zassert_equal(dev_actual_clk_src, RCC_SDIOCLKSOURCE_SYSCLK,
				"Expected SDMMC src: SYSCLK (0x%lx). Actual src: 0x%x",
				RCC_SDIOCLKSOURCE_SYSCLK, dev_actual_clk_src);
	} else {
		zassert_true(0, "Unexpected domain clk (0x%x)", dev_actual_clk_src);
	}

	/* Test get_rate(srce clk) */
	if (pclken[1].bus == STM32_SRC_CK48) {
		/* Get the CK48M source : PLL Q or PLLI2S Q */
		if (LL_RCC_GetCK48MClockSource(LL_RCC_CK48M_CLKSOURCE) ==
				LL_RCC_CK48M_CLKSOURCE_PLL) {
			/* Get the PLL Q freq. No HAL macro for that */
			TC_PRINT("SDMMC sourced by PLLQ at ");
			dev_actual_clk_freq = __LL_RCC_CALC_PLLCLK_48M_FREQ(HSE_VALUE,
							    LL_RCC_PLLI2S_GetDivider(),
							    LL_RCC_PLLI2S_GetN(),
							    LL_RCC_PLLI2S_GetQ()
							    );
		} else {
			/* Get the I2S PLL Q freq. No HAL macro for that */
			dev_actual_clk_freq = __LL_RCC_CALC_PLLI2S_48M_FREQ(HSE_VALUE,
							    LL_RCC_PLLI2S_GetDivider(),
							    LL_RCC_PLLI2S_GetN(),
							    LL_RCC_PLLI2S_GetQ()
							    );
			TC_PRINT("SDMMC sourced by PLLI2SQ at ");
		}

		TC_PRINT("%d Hz\n", dev_actual_clk_freq);
		r = 0;

	} else if (pclken[1].bus == STM32_SRC_SYSCLK) {
		dev_actual_clk_freq = HAL_RCC_GetSysClockFreq();
		TC_PRINT(" STM32_SRC_SYSCLK at %d\n", dev_actual_clk_freq);
	} else {
		r = -127;
	}

	zassert_true((r == 0), "Could not get SDMMC clk srce freq");

	r = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[1],
				&dev_dt_clk_freq);

	zassert_equal(dev_dt_clk_freq, dev_actual_clk_freq,
			"Expected freq: %d Hz. Actual clk: %d Hz",
			dev_dt_clk_freq, dev_actual_clk_freq);

	TC_PRINT("SDMMC clock rate: %d Hz\n", dev_dt_clk_freq);

	/* Test clock_off(gating clk) */
	r = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &pclken[0]);
	zassert_true((r == 0), "Could not disable SDMMC gating clk");

	zassert_true(!__HAL_RCC_SDIO_IS_CLK_ENABLED(), "SDMMC gating clk should be off");
	TC_PRINT("SDMMC gating clk off\n");
}
#endif
