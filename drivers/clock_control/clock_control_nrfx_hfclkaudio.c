/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "clock_control_nrfx_common.h"
#include <nrfx_clock_hfclkaudio.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(clock_control_hfclkaudio, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock_hfclkaudio

#define CLOCK_DEVICE_HFCLKAUDIO DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclkaudio)
#define CLOCK_NODE_HFCLKAUDIO DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_hfclkaudio)

static void clock_event_handler(void)
{
	common_clkstarted_handle(CLOCK_DEVICE_HFCLKAUDIO);
}

static void hfclkaudio_init(void)
{
#if DT_NODE_HAS_PROP(CLOCK_NODE_HFCLKAUDIO, hfclkaudio_frequency)
	const uint32_t frequency = DT_PROP(CLOCK_NODE_HFCLKAUDIO, hfclkaudio_frequency);
	/* As specified in the nRF5340 PS:
	 *
	 * FREQ_VALUE = 2^16 * ((12 * f_out / 32M) - 4)
	 */
	const uint32_t freq_value = (uint32_t)((384ULL * frequency) / 15625) - 262144;

#if NRF_CLOCK_HAS_HFCLKAUDIO
	nrf_clock_hfclkaudio_config_set(NRF_CLOCK, freq_value);
#else
#error "hfclkaudio-frequency specified but HFCLKAUDIO clock is not present."
#endif /* NRF_CLOCK_HAS_HFCLKAUDIO */
#endif
}

static int clk_init(const struct device *dev)
{
	if (nrfx_clock_hfclkaudio_init(clock_event_handler) != 0) {
		return -EIO;
	}

	hfclkaudio_init();

	return common_clk_init(dev);
}

CLOCK_CONTROL_NRFX_IRQ_HANDLERS_ITERABLE(clock_control_nrfx_hfclkaudio,
					&nrfx_clock_hfclkaudio_irq_handler);

extern struct nrf_clock_control_driver_api common_clock_control_api;

static common_clock_data_t data;

static const common_clock_config_t config = {
	.start = nrfx_clock_hfclkaudio_start,
	.stop = nrfx_clock_hfclkaudio_stop,
};

DEVICE_DT_DEFINE(CLOCK_NODE_HFCLKAUDIO, clk_init, NULL, &data, &config, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &common_clock_control_api);
