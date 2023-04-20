/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_w1

/*
 * This is not a real 1-Wire driver. It is only used to instantiate struct
 * devices for the "vnd,w1" devicetree compatibe used in test code.
 */
#include <zephyr/drivers/w1.h>

struct w1_vnd_config {
	/** w1 master config, common to all drivers */
	struct w1_master_config master_config;
};

struct w1_vnd_data {
	/** w1 master data, common to all drivers */
	struct w1_master_data master_data;
};

static int w1_vnd_reset_bus(const struct device *dev)
{
	return -ENOTSUP;
}

static int w1_vnd_read_bit(const struct device *dev)
{
	return -ENOTSUP;
}

static int w1_vnd_write_bit(const struct device *dev, const bool bit)
{
	return -ENOTSUP;
}

static int w1_vnd_read_byte(const struct device *dev)
{
	return -ENOTSUP;
}

static int w1_vnd_write_byte(const struct device *dev, const uint8_t byte)
{
	return -ENOTSUP;
}

static int w1_vnd_configure(const struct device *dev,
			    enum w1_settings_type type, uint32_t value)
{
	return -ENOTSUP;
}

static const struct w1_driver_api w1_vnd_api = {
	.reset_bus = w1_vnd_reset_bus,
	.read_bit = w1_vnd_read_bit,
	.write_bit = w1_vnd_write_bit,
	.read_byte = w1_vnd_read_byte,
	.write_byte = w1_vnd_write_byte,
	.configure = w1_vnd_configure,
};

#define W1_VND_INIT(n)							\
static const struct w1_vnd_config w1_vnd_cfg_##inst = {			\
	.master_config.slave_count = W1_INST_SLAVE_COUNT(inst)		\
};									\
static struct w1_vnd_data w1_vnd_data_##inst = {};			\
DEVICE_DT_INST_DEFINE(n, NULL, NULL, &w1_vnd_data_##inst,		\
		      &w1_vnd_cfg_##inst, POST_KERNEL,			\
		      CONFIG_W1_INIT_PRIORITY, &w1_vnd_api);

DT_INST_FOREACH_STATUS_OKAY(W1_VND_INIT)
