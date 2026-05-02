/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2022 Google, LLC
 *
 */

#include <zephyr/drivers/clock_control.h>

#define DT_DRV_COMPAT fixed_clock

struct fixed_rate_clock_config {
	uint32_t rate;
};

static int fixed_rate_clk_on(const struct device *dev,
			     clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

static int fixed_rate_clk_off(const struct device *dev,
			      clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

static enum clock_control_status fixed_rate_clk_get_status(const struct device *dev,
							   clock_control_subsys_t sys)
{
	return CLOCK_CONTROL_STATUS_ON;
}

static int fixed_rate_clk_get_rate(const struct device *dev,
				   clock_control_subsys_t sys,
				   uint32_t *rate)
{
	const struct fixed_rate_clock_config *config = dev->config;

	ARG_UNUSED(sys);

	*rate = config->rate;
	return 0;
}

static DEVICE_API(clock_control, fixed_rate_clk_api) = {
	.on = fixed_rate_clk_on,
	.off = fixed_rate_clk_off,
	.get_status = fixed_rate_clk_get_status,
	.get_rate = fixed_rate_clk_get_rate
};

static int fixed_rate_clk_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#define FIXED_CLK_INIT(idx) \
	static const struct fixed_rate_clock_config fixed_rate_clock_config_##idx = { \
		.rate = DT_INST_PROP(idx, clock_frequency),                           \
	};                                                                            \
	DEVICE_DT_INST_DEFINE(idx,                                                    \
		fixed_rate_clk_init,                                                  \
		NULL, NULL,                                                           \
		&fixed_rate_clock_config_##idx,                                       \
		PRE_KERNEL_1,                                                         \
		CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                                   \
		&fixed_rate_clk_api                                                   \
	);
DT_INST_FOREACH_STATUS_OKAY(FIXED_CLK_INIT)
