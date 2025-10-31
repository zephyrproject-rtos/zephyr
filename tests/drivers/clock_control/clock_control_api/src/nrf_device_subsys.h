/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "device_subsys.h"
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#if NRF_CLOCK_HAS_HFCLK
static const struct device_subsys_data subsys_data_hfclk[] = {
	{
		.subsys = CLOCK_CONTROL_NRF_SUBSYS_HF,
		.startup_us = CONFIG_TEST_NRF_HF_STARTUP_TIME_US
	}
};
#endif /* NRF_CLOCK_HAS_HFCLK */
#if NRF_CLOCK_HAS_XO
static const struct device_subsys_data subsys_data_xo[] = {
	{
		.subsys = CLOCK_CONTROL_NRF_SUBSYS_HF,
		.startup_us =  CONFIG_TEST_NRF_HF_STARTUP_TIME_US
	}
};
#endif /* NRF_CLOCK_HAS_XO */
#if !defined(CONFIG_SOC_NRF52832)
static const struct device_subsys_data subsys_data_lfclk[] = {
	{
		.subsys = CLOCK_CONTROL_NRF_SUBSYS_LF,
		.startup_us = (CLOCK_CONTROL_NRF_K32SRC ==
			    NRF_CLOCK_LFCLK_RC) ? 1000 : 500000
	}
};
#endif /* !defined(CONFIG_SOC_NRF52832) */
#if NRF_CLOCK_HAS_HFCLK192M
static const struct device_subsys_data subsys_data_hfclk192m[] = {
	{
		.subsys = CLOCK_CONTROL_NRF_SUBSYS_HF192M,
		.startup_us = 5
	}
};
#endif /* NRF_CLOCK_HAS_HFCLK192M */
#if NRF_CLOCK_HAS_XO24M
static const struct device_subsys_data subsys_data_xo24m[] = {
	{
		.subsys = CLOCK_CONTROL_NRF_SUBSYS_HF24M,
		.startup_us = 5
	}
};
#endif /* NRF_CLOCK_HAS_XO24M */

static const struct device_data devices[] = {
#if NRF_CLOCK_HAS_HFCLK
	{
		.dev = DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk),
		.subsys_data =  subsys_data_hfclk,
		.subsys_cnt = ARRAY_SIZE(subsys_data_hfclk)
	},
#endif /* NRF_CLOCK_HAS_HFCLK */
#if NRF_CLOCK_HAS_XO
	{
		.dev = DEVICE_DT_GET_ONE(nordic_nrf_clock_xo),
		.subsys_data =  subsys_data_xo,
		.subsys_cnt = ARRAY_SIZE(subsys_data_xo)
	},
#endif /* NRF_CLOCK_HAS_XO */
#if !defined(CONFIG_SOC_NRF52832)
	/* On nrf52832 LF clock cannot be stopped because it leads
	 * to RTC COUNTER register reset and that is unexpected by
	 * system clock which is disrupted and may hang in the test.
	 */
	{
		.dev = DEVICE_DT_GET_ONE(nordic_nrf_clock_lfclk),
		.subsys_data =  subsys_data_lfclk,
		.subsys_cnt = ARRAY_SIZE(subsys_data_lfclk)
	},
#endif /* !defined(CONFIG_SOC_NRF52832) */
#if NRF_CLOCK_HAS_HFCLK192M
	{
		.dev = DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk192m),
		.subsys_data =  subsys_data_hfclk192m,
		.subsys_cnt = ARRAY_SIZE(subsys_data_hfclk192m)
	},
#endif /* NRF_CLOCK_HAS_HFCLK192M */
#if NRF_CLOCK_HAS_XO24M
	{
		.dev = DEVICE_DT_GET_ONE(nordic_nrf_clock_xo24m),
		.subsys_data =  subsys_data_xo24m,
		.subsys_cnt = ARRAY_SIZE(subsys_data_xo24m)
	},
#endif /* NRF_CLOCK_HAS_XO24M */
};
