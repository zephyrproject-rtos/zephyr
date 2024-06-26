/*
 * Copyright (c) 2023, Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp3221

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MCP3221, CONFIG_SENSOR_LOG_LEVEL);

struct mcp3221_data {
	uint32_t voltage;
};

struct mcp3221_config {
	const struct i2c_dt_spec bus;
};

static int mcp3221_read(const struct device *dev, uint8_t *buf, int size)
{
	const struct mcp3221_config *cfg = dev->config;

	return i2c_read_dt(&cfg->bus, buf, size);
}

static int mcp3221_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mcp3221_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	uint8_t buf[2];
	int ret = mcp3221_read(dev, buf, sizeof(buf));
	if (ret < 0) {
		data->voltage = ~0;
		return ret;
	}

	data->voltage = (buf[0] << 8) | buf[1];

	/* (3.3V * 1000000) / 4096 = 806 */
	data->voltage *= 806;

	return 0;
}

static int mcp3221_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct mcp3221_data *data = dev->data;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	if (data->voltage == ~0) {
		return -EINVAL;
	}

	val->val1 = data->voltage / 1000000;
	val->val2 = data->voltage % 1000000;

	return 0;
}

static const struct sensor_driver_api mcp3221_api = {
	.sample_fetch = mcp3221_sample_fetch,
	.channel_get = mcp3221_channel_get,
};

static int mcp3221_init(const struct device *dev)
{
	const struct mcp3221_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("mcp3221 i2c bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	uint8_t buf[2];
	int ret = mcp3221_read(dev, buf, sizeof(buf));

	return ret;
}

#define MCP3221_DEFINE(id)                                                                         \
	static struct mcp3221_data mcp3221_data_##id;                                              \
                                                                                                   \
	static const struct mcp3221_config mcp3221_config_##id = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(id),                                                   \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(id, mcp3221_init, NULL, &mcp3221_data_##id,                   \
				     &mcp3221_config_##id, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &mcp3221_api);

DT_INST_FOREACH_STATUS_OKAY(MCP3221_DEFINE)
