/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_i2s

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>

static int vnd_i2s_configure(const struct device *dev, enum i2s_dir dir,
			     const struct i2s_config *i2s_cfg)
{
	return -ENOTSUP;
}

static const struct i2s_config *vnd_i2s_config_get(const struct device *dev, enum i2s_dir dir)
{
	return NULL;
}

static int vnd_i2s_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	return -ENOTSUP;
}

static int vnd_i2s_read(const struct device *dev, void **mem_block, size_t *size)
{
	return -ENOTSUP;
}

static int vnd_i2s_write(const struct device *dev, void *mem_block, size_t size)
{
	return -ENOTSUP;
}

static DEVICE_API(i2s, vnd_i2s_driver_api) = {
	.configure = vnd_i2s_configure,
	.config_get = vnd_i2s_config_get,
	.trigger = vnd_i2s_trigger,
	.read = vnd_i2s_read,
	.write = vnd_i2s_write,
};

static int vnd_i2s_init(const struct device *dev)
{
	return 0;
}

#define VND_I2S_INIT(index)                                                                        \
	DEVICE_DT_INST_DEFINE(index, &vnd_i2s_init, NULL, NULL, NULL, POST_KERNEL,                 \
			      CONFIG_I2S_INIT_PRIORITY, &vnd_i2s_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VND_I2S_INIT)
