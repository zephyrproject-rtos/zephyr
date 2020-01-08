/*
 * Copyright (c) 2020 Intive
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <sys/byteorder.h>

#include "isl29125.h"

LOG_MODULE_REGISTER(ISL29125, CONFIG_SENSOR_LOG_LEVEL);

static u8_t isl29125_read8(struct device *i2c_dev, u8_t reg)
{
	u8_t buf;

	if (i2c_reg_read_byte(i2c_dev, ISL29125_I2C_ADDR, reg, &buf) < 0) {
		LOG_ERR("Error reading register %d at Addr:0x%x", reg,
			ISL29125_I2C_ADDR);
	}
	return buf;
}

static u16_t isl29125_read16(struct device *i2c_dev, u8_t reg)
{
	u8_t buf[2];

	if (i2c_write_read(i2c_dev, ISL29125_I2C_ADDR, &reg, 1, &buf, 2) < 0) {
		LOG_ERR("Error reading register %d at Addr:0x%x", reg,
			ISL29125_I2C_ADDR);
	}
	return sys_le16_to_cpu(buf[0] | (buf[1] << 8));
}

static void isl29125_write8(struct device *i2c_dev, u8_t reg, u8_t data)
{
	if (i2c_reg_write_byte(i2c_dev, ISL29125_I2C_ADDR, reg, data) != 0) {
		LOG_ERR("Error writing register %d at Addr:0x%x", reg,
			ISL29125_I2C_ADDR);
	}
}

static u16_t isl29125_read_red(struct device *i2c_dev)
{
	return isl29125_read16(i2c_dev, ISL29125_RED_L);
}

static u16_t isl29125_read_green(struct device *i2c_dev)
{
	return isl29125_read16(i2c_dev, ISL29125_GREEN_L);
}

static u16_t isl29125_read_blue(struct device *i2c_dev)
{
	return isl29125_read16(i2c_dev, ISL29125_BLUE_L);
}

static int isl29125_reset(struct device *i2c_dev)
{
	u8_t data = 0;

	/* reset to defaults */
	isl29125_write8(i2c_dev, ISL29125_DEVICE_ID, 0x46);

	/* Check reset */
	data = isl29125_read8(i2c_dev, ISL29125_CONFIG_1);
	data |= isl29125_read8(i2c_dev, ISL29125_CONFIG_2);
	data |= isl29125_read8(i2c_dev, ISL29125_CONFIG_3);
	data |= isl29125_read8(i2c_dev, ISL29125_STATUS);

	return data != 0 ? -EIO : 0;
}

static int isl29125_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct isl29125_data *drv_data = dev->driver_data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_RED &&
	    chan != SENSOR_CHAN_GREEN && chan != SENSOR_CHAN_BLUE) {
		return -EINVAL;
	}

	if (chan == SENSOR_CHAN_RED || chan == SENSOR_CHAN_ALL) {
		drv_data->r = isl29125_read_red(drv_data->i2c);
	}
	if (chan == SENSOR_CHAN_GREEN || chan == SENSOR_CHAN_ALL) {
		drv_data->g = isl29125_read_green(drv_data->i2c);
	}
	if (chan == SENSOR_CHAN_BLUE || chan == SENSOR_CHAN_ALL) {
		drv_data->b = isl29125_read_blue(drv_data->i2c);
	}
	LOG_DBG("rgb: %u, %u, %u", drv_data->r, drv_data->g, drv_data->b);
	return 0;
}

static int isl29125_channel_get(struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct isl29125_data *drv_data = dev->driver_data;

	val->val2 = 0;
	switch (chan) {
	case SENSOR_CHAN_RED:
		val->val1 = drv_data->r;
		break;

	case SENSOR_CHAN_GREEN:
		val->val1 = drv_data->g;
		break;

	case SENSOR_CHAN_BLUE:
		val->val1 = drv_data->b;
		break;

	default:
		val->val1 = 0;
		break;
	}

	return 0;
}

int isl29125_set_config(struct isl29125_data *drv_data)
{
	/* Set configuration registers */
	isl29125_write8(drv_data->i2c, ISL29125_CONFIG_1,
			drv_data->dev_config_1);
	isl29125_write8(drv_data->i2c, ISL29125_CONFIG_2,
			drv_data->dev_config_2);
	isl29125_write8(drv_data->i2c, ISL29125_CONFIG_3,
			drv_data->dev_config_3);

	/* Check if configurations were set correctly */
	if (isl29125_read8(drv_data->i2c, ISL29125_CONFIG_1) !=
	    drv_data->dev_config_1 ||
	    isl29125_read8(drv_data->i2c, ISL29125_CONFIG_2) !=
	    drv_data->dev_config_2 ||
	    isl29125_read8(drv_data->i2c, ISL29125_CONFIG_3) !=
	    drv_data->dev_config_3) {
		return -EIO;
	}
	return 0;
}

static const struct sensor_driver_api isl29125_driver_api = {
#if CONFIG_ISL29125_TRIGGER
	.attr_set = &isl29125_attr_set,
	.trigger_set = &isl29125_trigger_set,
#endif
	.sample_fetch = isl29125_sample_fetch,
	.channel_get = isl29125_channel_get,
};

static int isl29125_init(struct device *dev)
{
	struct isl29125_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(DT_INST_0_ISIL_ISL29125_BUS_NAME);
	if (drv_data->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DT_INST_0_ISIL_ISL29125_BUS_NAME);
		return -EINVAL;
	}

	if (isl29125_read8(drv_data->i2c, ISL29125_DEVICE_ID) != 0x7D) {
		return -EIO;
	}

	int result = isl29125_reset(drv_data->i2c);

	if (result != 0) {
		return result;
	}

	/* Set to RGB mode, 10k lux, and high IR compensation */
	drv_data->dev_config_1 = ISL29125_CFG1_MODE_RGB | ISL29125_CFG1_10KLUX;
	drv_data->dev_config_2 = ISL29125_CFG2_IR_ADJUST_HIGH;
	drv_data->dev_config_3 = ISL29125_CFG_DEFAULT;

#ifdef CONFIG_ISL29125_TRIGGER
	if (isl29125_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupt.");
		return -EIO;
	}
#endif
	return isl29125_set_config(drv_data);
}

static struct isl29125_data isl29125_driver;

DEVICE_AND_API_INIT(isl29125, DT_INST_0_ISIL_ISL29125_LABEL, isl29125_init,
		    &isl29125_driver, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &isl29125_driver_api);
