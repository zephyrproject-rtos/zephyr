/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/clock_control_adsp.h>
#include <zephyr/drivers/clock_control.h>
#include "common_helpers.h"

static int cavs_clock_ctrl_set_rate(const struct device *clk,
				    clock_control_subsys_t sys,
				    clock_control_subsys_rate_t rate)
{
	uint32_t freq_idx = (uint32_t)rate;

	return adsp_clock_set_cpu_freq(freq_idx);
}

static DEVICE_API(clock_control, cavs_clock_api) = {
	.on = clock_control_always_running_clk_on,
	.off = clock_control_always_running_clk_off,
	.set_rate = cavs_clock_ctrl_set_rate,
};

DEVICE_DT_DEFINE(DT_NODELABEL(clkctl), NULL, NULL, NULL, NULL, POST_KERNEL,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &cavs_clock_api);
