/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_dac

#include <zephyr/kernel.h>
#include <zephyr/drivers/dac.h>

int vnd_dac_channel_setup(const struct device *dev, const struct dac_channel_cfg *channel_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_cfg);

	return -ENOTSUP;
}

int vnd_dac_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(value);

	return -ENOTSUP;
}

static const struct dac_driver_api vnd_dac_driver_api = {
	.channel_setup = vnd_dac_channel_setup,
	.write_value = vnd_dac_write_value,
};

static int vnd_dac_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#define VND_DAC_INIT(index)                                                                        \
	DEVICE_DT_INST_DEFINE(index, &vnd_dac_init, NULL, NULL, NULL, POST_KERNEL,                 \
			      CONFIG_DAC_INIT_PRIORITY, &vnd_dac_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VND_DAC_INIT)
