/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/ztest.h>

/* Not device related, but keep it to ensure core clock config is correct */
ZTEST(stm32n6_devices_clocks, test_cpuclk_freq)
{
	uint32_t cpuclk_freq = HAL_RCC_GetCpuClockFreq();

	zassert_equal(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, cpuclk_freq,
		      "Expected cpuclk_freq: %d. Actual cupclk_freq: %d",
		      CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, cpuclk_freq);
}

ZTEST_SUITE(stm32n6_devices_clocks, NULL, NULL, NULL, NULL, NULL);
