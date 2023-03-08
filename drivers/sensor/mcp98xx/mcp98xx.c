/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp98xx

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "mcp98xx.h"

LOG_MODULE_REGISTER(MCP98XX, CONFIG_SENSOR_LOG_LEVEL);

int mcp98xx_reg_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct mcp98xx_config *cfg = dev->config;
	int rc = i2c_write_read_dt(&cfg->i2c, &reg, sizeof(reg), val,
				   sizeof(*val));

	if (rc == 0) {
		*val = sys_be16_to_cpu(*val);
	}

	return rc;
}

int mcp98xx_reg_write_16bit(const struct device *dev, uint8_t reg,
			    uint16_t val)
{
	const struct mcp98xx_config *cfg = dev->config;

	uint8_t buf[3];

	buf[0] = reg;
	sys_put_be16(val, &buf[1]);

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

int mcp98xx_reg_write_8bit(const struct device *dev, uint8_t reg,
			   uint8_t val)
{
	const struct mcp98xx_config *cfg = dev->config;
	uint8_t buf[2] = {
		reg,
		val,
	};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

int mcp98xx_wait_tconv(uint8_t resolution) {
	// Use typical values unless max is specified.
	switch(resulution) {
		case 0b00: k_msleep(30); break;  /// TYP
		case 0b01: k_msleep(120); break; /// MAX
		case 0b10: k_msleep(130); break; /// TYP
		default:
		case 0b11: k_msleep(260); break; /// TYP
	}
}

static int mcp98xx_shutdown(const struct device *dev) {
	uint16_t value = 0;
	int result = mcp98xx_reg_read(dev, MCP98XX_REG_CONFIG, &value);
	if (result == 0) {
		value |= MCP98XX_REG_CONFIG_SHDN;
		return mcp98xx_reg_write_16bit(dev, MCP98XX_REG_CONFIG, value );
	}
	return result;
}

static int mcp98xx_wakeup(const struct device *dev) {
	uint16_t value = 0;
	int result = mcp98xx_reg_read(dev, MCP98XX_REG_CONFIG, &value);
	if (result == 0) {
		value &= ~MCP98XX_REG_CONFIG_SHDN;
		return mcp98xx_reg_write_16bit(dev, MCP98XX_REG_CONFIG, value );
	}
	return result;
}

static int mcp98xx_set_temperature_resolution(const struct device *dev,
					      uint8_t resolution)
{
	int result;
	uint16_t resolution_16bit = resolution;
#if CONFIG_MCP98XX_CHIP_MCP9844
	mcp98xx_shutdown(dev);
#endif
	result = mcp98xx_reg_write_16bit(dev, MCP98XX_REG_RESOLUTION, resolution_16bit);
	return result;
}

static int mcp98xx_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct mcp98xx_data *data = dev->data;
	int result;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

#if CONFIG_MCP98XX_CHIP_MCP9844
	mcp98xx_wakeup(dev);
#endif
	result = mcp98xx_reg_read(dev, MCP98XX_REG_TEMP_AMB, &data->reg_val);
	uint16_t dummy;
	result = mcp98xx_reg_read(dev, MCP98XX_REG_RESOLUTION, &dummy);
	LOG_INF("Read resolution: 0x%04x", dummy);

#if CONFIG_MCP98XX_CHIP_MCP9844
	mcp98xx_shutdown(dev);
#endif
	return result;
}

static int mcp98xx_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
	{
	const struct mcp98xx_data *data = dev->data;
	int temp = mcp98xx_temp_signed_from_reg(data->reg_val);

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP);

	val->val1 = temp / MCP98XX_TEMP_SCALE_CEL;
	temp -= val->val1 * MCP98XX_TEMP_SCALE_CEL;
	val->val2 = (temp * 1000000) / MCP98XX_TEMP_SCALE_CEL;

	return 0;
}

static const struct sensor_driver_api mcp98xx_api_funcs = {
	.sample_fetch = mcp98xx_sample_fetch,
	.channel_get = mcp98xx_channel_get,
#ifdef CONFIG_MCP98XX_TRIGGER
	.attr_set = mcp98xx_attr_set,
	.trigger_set = mcp98xx_trigger_set,
#endif /* CONFIG_MCP98XX_TRIGGER */
};

int mcp98xx_init(const struct device *dev)
{
	const struct mcp98xx_config *cfg = dev->config;
	int rc = 0;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	uint16_t dummy;
	mcp98xx_reg_read(dev, 0x07, &dummy);
	LOG_INF("DeviceID and Revision: 0x%04x", dummy);


	rc = mcp98xx_set_temperature_resolution(dev, cfg->resolution);
	if (rc) {
		LOG_ERR("Could not set the resolution of mcp98xx module");
		return rc;
	}

#ifdef CONFIG_MCP98XX_TRIGGER
	if (cfg->int_gpio.port) {
		rc = mcp98xx_setup_interrupt(dev);
	}
#endif /* CONFIG_MCP98XX_TRIGGER */

	return rc;
}

#define MCP98XX_DEFINE(inst)									\
	static struct mcp98xx_data mcp98xx_data_##inst;						\
												\
	static const struct mcp98xx_config mcp98xx_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.resolution = DT_INST_PROP(inst, resolution),					\
		IF_ENABLED(CONFIG_MCP98XX_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),))	\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, mcp98xx_init, NULL,						\
			      &mcp98xx_data_##inst, &mcp98xx_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &mcp98xx_api_funcs);			\

DT_INST_FOREACH_STATUS_OKAY(MCP98XX_DEFINE)
