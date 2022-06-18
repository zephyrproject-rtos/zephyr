/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2022 Google, LLC
 *
 */

#include <zephyr/drivers/clock_control.h>

#define DT_DRV_COMPAT fixed_factor_clock

struct fixed_factor_clock_config {
	const struct device *clk_dev;
	clock_control_subsys_t clk_subsys;
	uint32_t divider;
	uint32_t multiplier;
};

static int fixed_factor_clk_init(const struct device *dev)
{
	return 0;
}

static int fixed_factor_clk_on(const struct device *dev,
			       clock_control_subsys_t sys)
{
	const struct fixed_factor_clock_config *config = dev->config;

	ARG_UNUSED(sys);

	return clock_control_on(config->clk_dev, config->clk_subsys);
}

static int fixed_factor_clk_off(const struct device *dev,
				clock_control_subsys_t sys)
{
	const struct fixed_factor_clock_config *config = dev->config;

	ARG_UNUSED(sys);

	return clock_control_off(config->clk_dev, config->clk_subsys);
}

static enum clock_control_status fixed_factor_clk_get_status(const struct device *dev,
							     clock_control_subsys_t sys)
{
	const struct fixed_factor_clock_config *config = dev->config;

	return clock_control_get_status(config->clk_dev, config->clk_subsys);
}

static int fixed_factor_clk_get_rate(const struct device *dev,
				     clock_control_subsys_t sys,
				     uint32_t *rate)
{
	const struct fixed_factor_clock_config *config = dev->config;
	uint32_t input_rate;
	int err;

	ARG_UNUSED(sys);

	err = clock_control_get_rate(config->clk_dev, config->clk_subsys,
				     &input_rate);
	if (err) {
		return err;
	}

	*rate = ((uint64_t)input_rate * config->multiplier) / config->divider;

	return 0;
}

static const struct clock_control_driver_api fixed_factor_clk_api = {
	.on = fixed_factor_clk_on,
	.off = fixed_factor_clk_off,
	.get_status = fixed_factor_clk_get_status,
	.get_rate = fixed_factor_clk_get_rate
};

#define INPUT_SUBSYS_OR_ALL(idx)                                  \
	COND_CODE_1(DT_INST_NUM_CLOCKS(idx),                      \
		(DT_INST_PHA_BY_IDX_OR(idx, clocks, 0, name,      \
				      CLOCK_CONTROL_SUBSYS_ALL)), \
		    (CLOCK_CONTROL_SUBSYS_ALL))

#define FIXED_FACTOR_CLK_INIT(idx) \
	static const struct fixed_factor_clock_config fixed_factor_clock_config_##idx = { \
		.clk_dev = DEVICE_DT_GET(DT_INST_PHANDLE(idx, clocks)),                   \
		.clk_subsys = (clock_control_subsys_t)INPUT_SUBSYS_OR_ALL(idx),           \
		.multiplier = DT_INST_PROP(idx, clock_mult),                              \
		.divider = DT_INST_PROP(idx, clock_div),                                  \
	};                                                                                \
	DEVICE_DT_INST_DEFINE(idx,                                                        \
		fixed_factor_clk_init,                                                    \
		NULL, NULL,                                                               \
		&fixed_factor_clock_config_##idx,                                         \
		PRE_KERNEL_1,                                                             \
		CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                                       \
		&fixed_factor_clk_api                                                     \
	);
DT_INST_FOREACH_STATUS_OKAY(FIXED_FACTOR_CLK_INIT)
