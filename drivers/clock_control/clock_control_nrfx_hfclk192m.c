/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "nrf_clock_calibration.h"
#include "clock_control_nrfx_common.h"
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

static int clk_init(const struct device *dev)
{
	if (nrfx_clock_hfclk192m_init(clock_event_handler) != 0) {
		return -EIO;
	}

	return common_clk_init(dev);
}

CLOCK_CONTROL_NRFX_IRQ_HANDLERS_ITERABLE(clock_control_nrfx_hfclk192m,
					&nrfx_clock_hfclk192m_irq_handler);

extern struct nrf_clock_control_driver_api common_clock_control_api;

static common_clock_data_t data;

static const common_clock_config_t config = {
	.start = nrfx_clock_hfclk192m_start,
	.stop = nrfx_clock_hfclk192m_stop,
};

DEVICE_DT_DEFINE(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_hfclk192m), clk_init, NULL, &data,
		 &config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &common_clock_control_api);
