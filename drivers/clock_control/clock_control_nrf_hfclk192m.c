/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "nrf_clock_calibration.h"
#include "clock_control_nrf_common.h"
#include <nrfx_clock_hfclk192m.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(clock_control_hfclk192m, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock_hfclk192m

#define CLOCK_DEVICE_HFCLK192M DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk192m)

static void clock_event_handler(void)
{
	common_clkstarted_handle(CLOCK_DEVICE_HFCLK192M);
}

#if NRF_CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT

#define HFCLK192M_CLOCK_FREQUENCIES DT_INST_PROP(0, supported_clock_frequencies)

#define HFCLK192M_CLOCK_FREQUENCIES_SIZE DT_INST_PROP_LEN(0, supported_clock_frequencies)

static const uint32_t supported_frequencies[HFCLK192M_CLOCK_FREQUENCIES_SIZE] =
		HFCLK192M_CLOCK_FREQUENCIES;

static int hfclk192m_set_rate(const struct device *dev, clock_control_subsys_t sys,
			  clock_control_subsys_rate_t rate)
{
	uint32_t freq_hz = (uint32_t)(uintptr_t)rate;
	nrf_clock_hfclk_div_t div = UINT8_MAX;

	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	for (uint32_t i = 0; i < HFCLK192M_CLOCK_FREQUENCIES_SIZE; i++) {
		if (supported_frequencies[i] == freq_hz) {
			div = i;
			break;
		}
	}

	if (div == UINT8_MAX) {
		return -ENOTSUP;
	}

	if (nrfx_clock_hfclk192m_divider_get() == div) {
		return -EALREADY;
	}

	nrfx_clock_hfclk192m_divider_set(div);

	return 0;
}

static int hfclk192m_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	*rate = supported_frequencies[nrfx_clock_hfclk192m_divider_get()];

	return 0;
}

#endif /* NRF_CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT */

static int clk_init(const struct device *dev)
{
	if (nrfx_clock_hfclk192m_init(clock_event_handler) != 0) {
		return -EIO;
	}

	return common_clk_init(dev);
}

CLOCK_CONTROL_NRF_IRQ_HANDLERS_ITERABLE(clock_control_nrf_hfclk192m,
					&nrfx_clock_hfclk192m_irq_handler);


static common_clock_data_t data;

static const common_clock_config_t config = {
	.start = nrfx_clock_hfclk192m_start,
	.stop = nrfx_clock_hfclk192m_stop,
};


#if NRF_CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT

DEVICE_API(nrf_clock_control, hfclk192m_clock_control_api) = {
	.std_api = {
		.on = common_api_blocking_start,
		.off = common_api_stop,
		.async_on = common_api_start,
		.get_status = common_api_get_status,
		.get_rate = hfclk192m_get_rate,
		.set_rate = hfclk192m_set_rate,
	},
#if CONFIG_CLOCK_CONTROL_NRF_ONOFF
	.request = common_api_request,
	.release = common_api_release,
	.cancel_or_release = common_api_cancel_or_release,
#endif
};

DEVICE_DT_DEFINE(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_hfclk192m), clk_init, NULL, &data,
		 &config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &hfclk192m_clock_control_api);
#else

extern struct nrf_clock_control_driver_api common_clock_control_api;

DEVICE_DT_DEFINE(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_hfclk192m), clk_init, NULL, &data,
		 &config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &common_clock_control_api);
#endif
