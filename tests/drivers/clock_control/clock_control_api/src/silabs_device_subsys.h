/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "device_subsys.h"
#include <zephyr/drivers/clock_control/clock_control_silabs.h>

struct silabs_clock_control_cmu_config i2c0_clock = {
	.bus_clock = CLOCK_I2C0,
	.branch = CLOCK_BRANCH_LSPCLK,
};

struct silabs_clock_control_cmu_config wdog0_clock = {
	.bus_clock = CLOCK_WDOG0,
	.branch = CLOCK_BRANCH_WDOG0CLK,
};

static const struct device_subsys_data subsys_data[] = {
	{.subsys = (void *)&i2c0_clock},
	{.subsys = (void *)&wdog0_clock},
};

static const struct device_data devices[] = {
	{
		.dev = DEVICE_DT_GET_ONE(silabs_series_clock),
		.subsys_data =  subsys_data,
		.subsys_cnt = ARRAY_SIZE(subsys_data)
	}
};
