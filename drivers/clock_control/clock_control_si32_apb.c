/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si32_apb

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <SI32_CLKCTRL_A_Type.h>
#include <si32_device.h>

struct clock_control_si32_apb_config {
	const struct device *clock_dev;
	uint32_t divider;
};

static int clock_control_si32_apb_on(const struct device *dev, clock_control_subsys_t sys)
{
	return -ENOTSUP;
}

static int clock_control_si32_apb_off(const struct device *dev, clock_control_subsys_t sys)
{

	return -ENOTSUP;
}

static int clock_control_si32_apb_get_rate(const struct device *dev, clock_control_subsys_t sys,
					   uint32_t *rate)
{
	const struct clock_control_si32_apb_config *config = dev->config;
	const int ret = clock_control_get_rate(config->clock_dev, NULL, rate);

	if (ret) {
		return ret;
	}

	*rate /= config->divider;

	return 0;
}

static DEVICE_API(clock_control, clock_control_si32_apb_api) = {
	.on = clock_control_si32_apb_on,
	.off = clock_control_si32_apb_off,
	.get_rate = clock_control_si32_apb_get_rate,
};

static int clock_control_si32_apb_init(const struct device *dev)
{
	const struct clock_control_si32_apb_config *config = dev->config;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	if (config->divider == 1) {
		SI32_CLKCTRL_A_select_apb_divider_1(SI32_CLKCTRL_0);
	} else if (config->divider == 2) {
		SI32_CLKCTRL_A_select_apb_divider_2(SI32_CLKCTRL_0);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct clock_control_si32_apb_config config = {
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.divider = DT_PROP(DT_NODELABEL(clk_apb), divider),
};

DEVICE_DT_INST_DEFINE(0, clock_control_si32_apb_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_si32_apb_api);
