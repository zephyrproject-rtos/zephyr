/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "device_subsys.h"
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

static const struct device_subsys_data subsys_data[] = {
	{
		.subsys = CLOCK_CONTROL_NRF_SUBSYS_HF,
		.startup_us =
			IS_ENABLED(CONFIG_SOC_SERIES_NRF91X) ?
				3000 : 500
	},
#ifndef CONFIG_SOC_NRF52832
	/* On nrf52832 LF clock cannot be stopped because it leads
	 * to RTC COUNTER register reset and that is unexpected by
	 * system clock which is disrupted and may hang in the test.
	 */
	{
		.subsys = CLOCK_CONTROL_NRF_SUBSYS_LF,
		.startup_us = (CLOCK_CONTROL_NRF_K32SRC ==
			NRF_CLOCK_LFCLK_RC) ? 1000 : 500000
	}
#endif /* !CONFIG_SOC_NRF52832 */
};

static const struct device_data devices[] = {
	{
		.dev = DEVICE_DT_GET_ONE(nordic_nrf_clock),
		.subsys_data =  subsys_data,
		.subsys_cnt = ARRAY_SIZE(subsys_data)
	}
};
