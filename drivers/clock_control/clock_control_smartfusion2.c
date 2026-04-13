/*
 * Copyright (c) 2025 Bavariamatic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_smartfusion2_clock

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>

struct smartfusion2_clock_config {
	const uint32_t *rates;
	uint32_t rate_count;
};

static int smartfusion2_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

static int smartfusion2_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

static enum clock_control_status smartfusion2_clock_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return CLOCK_CONTROL_STATUS_ON;
}

static int smartfusion2_clock_get_rate(const struct device *dev, clock_control_subsys_t sys,
					       uint32_t *rate)
{
	const struct smartfusion2_clock_config *config = dev->config;
	uintptr_t clkid = (uintptr_t)sys;

	if (clkid >= config->rate_count) {
		return -EINVAL;
	}

	*rate = config->rates[clkid];
	return 0;
}

static DEVICE_API(clock_control, smartfusion2_clock_api) = {
	.on = smartfusion2_clock_on,
	.off = smartfusion2_clock_off,
	.get_status = smartfusion2_clock_get_status,
	.get_rate = smartfusion2_clock_get_rate,
};

static int smartfusion2_clock_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#define SMARTFUSION2_CLOCK_INIT(inst)                                                       \
	static const uint32_t smartfusion2_clock_rates_##inst[] =                            \
		DT_INST_PROP(inst, clock_frequencies);                                          \
	                                                                                       \
	static const struct smartfusion2_clock_config smartfusion2_clock_config_##inst = {    \
		.rates = smartfusion2_clock_rates_##inst,                                      \
		.rate_count = ARRAY_SIZE(smartfusion2_clock_rates_##inst),                     \
	};                                                                                     \
	                                                                                       \
	DEVICE_DT_INST_DEFINE(inst, smartfusion2_clock_init, NULL, NULL,                      \
			      &smartfusion2_clock_config_##inst, PRE_KERNEL_1,              \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &smartfusion2_clock_api);

DT_INST_FOREACH_STATUS_OKAY(SMARTFUSION2_CLOCK_INIT)