/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_cgc_subclk

#include <string.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>

struct clock_control_ra_subclk_cfg {
	uint32_t rate;
};

static int clock_control_renesas_ra_subclk_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return -ENOTSUP;
}

static int clock_control_renesas_ra_subclk_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return -ENOTSUP;
}

static int clock_control_renesas_ra_subclk_get_rate(const struct device *dev,
						    clock_control_subsys_t sys, uint32_t *rate)
{
	const struct clock_control_ra_subclk_cfg *config = dev->config;

	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	*rate = config->rate;
	return 0;
}

static DEVICE_API(clock_control, clock_control_renesas_ra_subclk_api) = {
	.on = clock_control_renesas_ra_subclk_on,
	.off = clock_control_renesas_ra_subclk_off,
	.get_rate = clock_control_renesas_ra_subclk_get_rate,
};

#define RENESAS_RA_SUBCLK_INIT(idx)                                                                \
	static const struct clock_control_ra_subclk_cfg clock_control_ra_subclk_cfg##idx = {       \
		.rate = DT_INST_PROP(idx, clock_frequency),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, NULL, NULL, NULL, &clock_control_ra_subclk_cfg##idx,            \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                    \
			      &clock_control_renesas_ra_subclk_api);

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RA_SUBCLK_INIT);
