/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "tmp007.h"

LOG_MODULE_REGISTER(TMP007, CONFIG_SENSOR_LOG_LEVEL);

int tmp007_reg_read(struct tmp007_data *drv_data, u8_t reg, u16_t *val)
{
	struct i2c_msg msgs[2] = {
		{
			.buf = &reg,
			.len = 1,
			.flags = I2C_MSG_WRITE | I2C_MSG_RESTART,
		},
		{
			.buf = (u8_t *)val,
			.len = 2,
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		},
	};

	if (i2c_transfer(drv_data->i2c, msgs, 2, TMP007_I2C_ADDRESS) < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

int tmp007_reg_write(struct tmp007_data *drv_data, u8_t reg, u16_t val)
{
	u8_t tx_buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 TMP007_I2C_ADDRESS);
}

int tmp007_reg_update(struct tmp007_data *drv_data, u8_t reg,
		      u16_t mask, u16_t val)
{
	u16_t old_val = 0U;
	u16_t new_val;

	if (tmp007_reg_read(drv_data, reg, &old_val) < 0) {
		return -EIO;
	}

	new_val = old_val & ~mask;
	new_val |= val & mask;

	return tmp007_reg_write(drv_data, reg, new_val);
}

static int tmp007_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct tmp007_data *drv_data = dev->driver_data;
	u16_t val;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (tmp007_reg_read(drv_data, TMP007_REG_TOBJ, &val) < 0) {
		return -EIO;
	}

	if (val & TMP007_DATA_INVALID_BIT) {
		return -EIO;
	}

	drv_data->sample = arithmetic_shift_right((s16_t)val, 2);

	return 0;
}

static int tmp007_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct tmp007_data *drv_data = dev->driver_data;
	s32_t uval;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	uval = (s32_t)drv_data->sample * TMP007_TEMP_SCALE;
	val->val1 = uval / 1000000;
	val->val2 = uval % 1000000;

	return 0;
}

static const struct sensor_driver_api tmp007_driver_api = {
#ifdef CONFIG_TMP007_TRIGGER
	.attr_set = tmp007_attr_set,
	.trigger_set = tmp007_trigger_set,
#endif
	.sample_fetch = tmp007_sample_fetch,
	.channel_get = tmp007_channel_get,
};

int tmp007_init(struct device *dev)
{
	struct tmp007_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_TMP007_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		LOG_DBG("Failed to get pointer to %s device!",
			    CONFIG_TMP007_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

#ifdef CONFIG_TMP007_TRIGGER
	if (tmp007_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

struct tmp007_data tmp007_driver;

DEVICE_AND_API_INIT(tmp007, CONFIG_TMP007_NAME, tmp007_init, &tmp007_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &tmp007_driver_api);
