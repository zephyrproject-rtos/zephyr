/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp9843

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(MCP9843, CONFIG_SENSOR_LOG_LEVEL);

#define MCP9843_REG_AMBIENT_TEMP 0x05
#define MCP9843_REG_ID_REVISION  0x07
#define MCP9843_REG_RESOLUTION   0x08

#define MCP9843_TEMP_SCALE_CEL 16

struct mcp9843_data {
	uint8_t temp_buf[2];
};

struct mcp9843_config {
	const struct i2c_dt_spec bus;
	uint8_t resolution;
};

static int mcp9843_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct mcp9843_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->bus, start, buf, size);
}

static int mcp9843_set_temperature_resolution(const struct device *dev, uint8_t resolution)
{
	const struct mcp9843_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->bus, MCP9843_REG_RESOLUTION, resolution);
}

static int mcp9843_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mcp9843_data *data = dev->data;
	uint8_t buf[2];
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	ret = mcp9843_reg_read(dev, MCP9843_REG_AMBIENT_TEMP, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read data");
		return ret;
	}

	data->temp_buf[0] = buf[0];	
	data->temp_buf[1] = buf[1];

	return 0;
}

static int mcp9843_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct mcp9843_data *data = dev->data;
	int temp;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}


	data->temp_buf[0] &= 0x1F; /* mask out flags */

	if ((data->temp_buf[0] & 0x10) == 0x10) {
		/* negative temperature */
		data->temp_buf[0] &= 0x0F;
		temp = sys_get_be16(data->temp_buf) - 4096;
	} else {
		/* positive temperature */
		temp = sys_get_be16(data->temp_buf);
	}

	val->val1 = temp / MCP9843_TEMP_SCALE_CEL;
	val->val2 = (temp * 1000000 / MCP9843_TEMP_SCALE_CEL) % 1000000;

	return 0;
}

static const struct sensor_driver_api mcp9843_api = {
	.sample_fetch = mcp9843_sample_fetch,
	.channel_get = mcp9843_channel_get,
};

static int mcp9843_init(const struct device *dev)
{
	const struct mcp9843_config *cfg = dev->config;
	uint8_t buf[2];
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("mcp9843 i2c bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	ret = mcp9843_reg_read(dev, MCP9843_REG_ID_REVISION, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read chip id");
		return ret;
	}
	LOG_DBG("id: 0x%02x version: 0x%02x", buf[0], buf[1]);

	ret = mcp9843_set_temperature_resolution(dev, cfg->resolution);
	if (ret < 0) {
		LOG_ERR("Failed to set temperature resolution");
		return ret;
	}

	return ret;
}

#define MCP9843_DEFINE(n)                                                                          \
	static struct mcp9843_data mcp9843_data_##n;                                               \
                                                                                                   \
	static const struct mcp9843_config mcp9843_config_##n = {                                  \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.resolution = DT_INST_PROP(n, resolution),                                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, mcp9843_init, NULL, &mcp9843_data_##n,                     \
				     &mcp9843_config_##n, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &mcp9843_api);

DT_INST_FOREACH_STATUS_OKAY(MCP9843_DEFINE)
