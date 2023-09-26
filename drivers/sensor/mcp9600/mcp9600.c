/*
 * Copyright (c) 2022, Maxmillion McLaughlin
 * Copyright (c) 2020, SER Consulting LLC / Steven Riedl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp9600

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MCP9600, CONFIG_SENSOR_LOG_LEVEL);

#define MCP9600_REG_TEMP_HOT 0x00

#define MCP9600_REG_TEMP_DIFF 0x01
#define MCP9600_REG_TEMP_COLD 0x02
#define MCP9600_REG_RAW_ADC   0x03

#define MCP9600_REG_STATUS 0x04

#define MCP9600_REG_TC_CONFIG  0x05
#define MCP9600_REG_DEV_CONFIG 0x06
#define MCP9600_REG_A1_CONFIG  0x07

#define MCP9600_REG_A2_CONFIG 0x08
#define MCP9600_REG_A3_CONFIG 0x09
#define MCP9600_REG_A4_CONFIG 0x0A
#define MCP9600_A1_HYST	      0x0B

#define MCP9600_A2_HYST	 0x0C
#define MCP9600_A3_HYST	 0x0D
#define MCP9600_A4_HYST	 0x0E
#define MCP9600_A1_LIMIT 0x0F

#define MCP9600_A2_LIMIT	0x10
#define MCP9600_A3_LIMIT	0x11
#define MCP9600_A4_LIMIT	0x12
#define MCP9600_REG_ID_REVISION 0x13

struct mcp9600_data {
	int32_t temp;
};

struct mcp9600_config {
	const struct i2c_dt_spec bus;
};

static int mcp9600_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct mcp9600_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->bus, start, buf, size);
}

static int mcp9600_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mcp9600_data *data = dev->data;
	uint8_t buf[2];
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	/* read signed 16 bit double-buffered register value */
	ret = mcp9600_reg_read(dev, MCP9600_REG_TEMP_HOT, buf, sizeof(buf));
	if (ret < 0) {
		data->temp = 1;
		return ret;
	}

	/* device's hot junction register is a signed int */
	data->temp = (int32_t)(int16_t)(buf[0] << 8) | buf[1];

	/* 0.0625C resolution per LSB */
	data->temp *= 62500;

	return 0;
}

static int mcp9600_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct mcp9600_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	if (data->temp == 1) {
		return -EINVAL;
	}

	val->val1 = data->temp / 1000000;
	val->val2 = data->temp % 1000000;

	return 0;
}

static const struct sensor_driver_api mcp9600_api = {
	.sample_fetch = mcp9600_sample_fetch,
	.channel_get = mcp9600_channel_get,
};

static int mcp9600_init(const struct device *dev)
{
	const struct mcp9600_config *cfg = dev->config;
	uint8_t buf[2];
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("mcp9600 i2c bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	ret = mcp9600_reg_read(dev, MCP9600_REG_ID_REVISION, buf, sizeof(buf));
	LOG_DBG("id: 0x%02x version: 0x%02x", buf[0], buf[1]);

	return ret;
}

#define MCP9600_DEFINE(id)                                                                         \
	static struct mcp9600_data mcp9600_data_##id;                                              \
                                                                                                   \
	static const struct mcp9600_config mcp9600_config_##id = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(id),                                                   \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(id, mcp9600_init, NULL, &mcp9600_data_##id,                   \
				     &mcp9600_config_##id, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &mcp9600_api);

DT_INST_FOREACH_STATUS_OKAY(MCP9600_DEFINE)
