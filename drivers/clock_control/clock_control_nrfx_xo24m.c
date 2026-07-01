/*
 * Copyright (c) 2016-2025 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "clock_control_nrfx_common.h"
#include <nrfx_clock_xo24m.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(clock_control_xo24m, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock_xo24m

#define CLOCK_DEVICE_XO24M DEVICE_DT_GET_ONE(nordic_nrf_clock_xo24m)

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_clock_xo)
#define CLOCK_DEVICE_XO DEVICE_DT_GET_ONE(nordic_nrf_clock_xo)

static struct onoff_client xo24m_cli;
#endif

static void xo24m_start(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_clock_xo)
	int ret;

	/* Request HFCLK to be when HFCLK24M is running.
	 * HFCLK (XO) is automatically requested when XO24M is started but it's
	 * easier to manage from software perspective if XO is explicitly requested.
	 */
	sys_notify_init_spinwait(&xo24m_cli.notify);
	ret = onoff_request(&((common_clock_data_t *)CLOCK_DEVICE_XO->data)->mgr, &xo24m_cli);
	if (ret < 0) {
		LOG_ERR("Failed to request XO for XO24M: %d", ret);
		__ASSERT_NO_MSG(false);
	}
#endif
	nrfx_clock_xo24m_start();
}

static void xo24m_stop(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_clock_xo)
	(void)onoff_cancel_or_release(&((common_clock_data_t *)CLOCK_DEVICE_XO->data)->mgr, 
	&xo24m_cli);
#endif
	nrfx_clock_xo24m_stop();
}

static void clock_event_handler(void)
{
	common_clkstarted_handle(CLOCK_DEVICE_XO24M);
}

static int clk_init(const struct device *dev)
{
	if (nrfx_clock_xo24m_init(clock_event_handler) != 0) {
		return -EIO;
	}

	return common_clk_init(dev);
}

CLOCK_CONTROL_NRFX_IRQ_HANDLERS_ITERABLE(clock_control_nrfx_xo24m, &nrfx_clock_xo24m_irq_handler);

extern struct nrf_clock_control_driver_api common_clock_control_api;

static common_clock_data_t data;

static const common_clock_config_t config = {
	.start = xo24m_start,
	.stop = xo24m_stop,
};

DEVICE_DT_DEFINE(DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_clock_xo24m), clk_init, NULL, &data,
		&config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		&common_clock_control_api);
