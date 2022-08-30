/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/clock_control_cavs.h>
#include <zephyr/drivers/clock_control.h>

static int cavs_clock_ctrl_set_rate(const struct device *clk,
				    clock_control_subsys_t sys,
				    clock_control_subsys_rate_t rate)
{
	uint32_t freq_idx = (uint32_t)rate;

	return cavs_clock_set_freq(freq_idx);
}

static int cavs_clock_ctrl_init(const struct device *dev)
{
	/* Nothing to do. All initialisation should've been handled
	 * by SOC level driver.
	 */
	return 0;
}

static const struct clock_control_driver_api cavs_clock_api = {
	.set_rate = cavs_clock_ctrl_set_rate
};

DEVICE_DT_DEFINE(DT_NODELABEL(clkctl), &cavs_clock_ctrl_init, NULL,
		 NULL, NULL, POST_KERNEL,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &cavs_clock_api);
