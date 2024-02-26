/*
 * Copyright (c) 2024 Robert Kowalewski
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp9800

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "mcp9800.h"

LOG_MODULE_REGISTER(MCP9800, CONFIG_SENSOR_LOG_LEVEL);

int mcp9800_reg_read_16bit(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct mcp9800_config *config = dev->config;
	int rc = i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), val,
				   sizeof(*val));

	if (rc == 0) {
		*val = sys_be16_to_cpu(*val);
	}

	return rc;
}

int mcp9800_reg_read_8bit(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct mcp9800_config *config = dev->config;

	return i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), val, sizeof(*val));
}

int mcp9800_reg_write_16bit(const struct device *dev, uint8_t reg,
				uint16_t val)
{
	const struct mcp9800_config *config = dev->config;

	uint8_t buf[3];

	buf[0] = reg;
	sys_put_be16(val, &buf[1]);

	return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

int mcp9800_reg_write_8bit(const struct device *dev, uint8_t reg,
			   uint8_t val)
{
	const struct mcp9800_config *config = dev->config;
	uint8_t buf[2] = {
		reg,
		val,
	};

	return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static int mcp9800_set_temperature_resolution(const struct device *dev,
						  uint8_t resolution)
{
	uint8_t reg;

	int rc = mcp9800_reg_read_8bit(dev, MCP9800_REG_CONFIG, &reg);

	if (rc < 0) {
		return rc;
	}

	reg &= ~MCP9800_CONFIG_RESOLUTION_MASK;
	reg |= (resolution << MCP9800_CONFIG_RESOLUTION_SHIFT);

	return mcp9800_reg_write_8bit(dev, MCP9800_REG_CONFIG, reg);
}

static int mcp9800_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct mcp9800_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	return mcp9800_reg_read_16bit(dev, MCP9800_REG_TEMP_AMB, &data->reg_val);
}

static int mcp9800_channel_get(const struct device *dev,
				   enum sensor_channel chan,
				   struct sensor_value *val)
{
	const struct mcp9800_data *data = dev->data;
	int temp = mcp9800_temp_signed_from_reg(data->reg_val);

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = temp / MCP9800_TEMP_SCALE_CEL;
	temp -= val->val1 * MCP9800_TEMP_SCALE_CEL;
	val->val2 = (temp * 1000000) / MCP9800_TEMP_SCALE_CEL;

	return 0;
}

static const struct sensor_driver_api mcp9800_api_funcs = {
	.sample_fetch = mcp9800_sample_fetch,
	.channel_get = mcp9800_channel_get,
#ifdef CONFIG_MCP9800_TRIGGER
	.attr_set = mcp9800_attr_set,
	.trigger_set = mcp9800_trigger_set,
#endif /* CONFIG_MCP9800_TRIGGER */
};

int mcp9800_init(const struct device *dev)
{
	const struct mcp9800_config *config = dev->config;
	int rc = 0;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	rc = mcp9800_set_temperature_resolution(dev, config->resolution);
	if (rc < 0) {
		LOG_ERR("Could not set the resolution of mcp9800 module");
		return rc;
	}

#ifdef CONFIG_MCP9800_TRIGGER
	if (config->int_gpio.port) {
		rc = mcp9800_setup_interrupt(dev);
	}
#endif /* CONFIG_MCP9800_TRIGGER */

	return rc;
}

#define MCP9800_DEFINE(inst)									\
	static struct mcp9800_data mcp9800_data_##inst;						\
	static const struct mcp9800_config mcp9800_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.resolution = DT_INST_PROP(inst, resolution),					\
		IF_ENABLED(CONFIG_MCP9800_TRIGGER,						\
				(.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mcp9800_init, NULL,					\
				&mcp9800_data_##inst, &mcp9800_config_##inst, POST_KERNEL,	\
				CONFIG_SENSOR_INIT_PRIORITY, &mcp9800_api_funcs);		\

DT_INST_FOREACH_STATUS_OKAY(MCP9800_DEFINE)
