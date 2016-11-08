/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <device.h>
#include <i2c.h>
#include <sensor.h>
#include <misc/__assert.h>

#include "max44009.h"

static int max44009_reg_read(struct max44009_data *drv_data, uint8_t reg,
			     uint8_t *val, bool send_stop)
{
	struct i2c_msg msgs[2] = {
		{
			.buf = &reg,
			.len = 1,
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = val,
			.len = 1,
			.flags = I2C_MSG_READ,
		},
	};

	if (send_stop) {
		msgs[1].flags |= I2C_MSG_STOP;
	}

	if (i2c_transfer(drv_data->i2c, msgs, 2, MAX44009_I2C_ADDRESS) != 0) {
		return -EIO;
	}

	return 0;
}

static int max44009_reg_write(struct max44009_data *drv_data, uint8_t reg,
			      uint8_t val)
{
	uint8_t tx_buf[2] = {reg, val};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 MAX44009_I2C_ADDRESS);
}

static int max44009_reg_update(struct max44009_data *drv_data, uint8_t reg,
			       uint8_t mask, uint8_t val)
{
	uint8_t old_val = 0;
	uint8_t new_val = 0;

	if (max44009_reg_read(drv_data, reg, &old_val, true) != 0) {
		return -EIO;
	}

	new_val = old_val & ~mask;
	new_val |= val & mask;

	return max44009_reg_write(drv_data, reg, new_val);
}

static int max44009_attr_set(struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	struct max44009_data *drv_data = dev->driver_data;
	uint8_t value;
	uint32_t cr;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->type != SENSOR_VALUE_TYPE_INT_PLUS_MICRO) {
			return -ENOTSUP;
		}

		/* convert rate to mHz */
		cr = val->val1 * 1000 + val->val2 / 1000;

		/* the sensor supports 1.25Hz or continuous conversion */
		switch (cr) {
		case 1250:
			value = 0;
			break;
		default:
			value = MAX44009_CONTINUOUS_SAMPLING;
		}

		if (max44009_reg_update(drv_data, MAX44009_REG_CONFIG,
					MAX44009_SAMPLING_CONTROL_BIT,
					value) != 0) {
			SYS_LOG_DBG("Failed to set attribute!");
			return -EIO;
		}

		return 0;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int max44009_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct max44009_data *drv_data = dev->driver_data;
	uint8_t val_h, val_l;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT);

	drv_data->sample = 0;

	if (max44009_reg_read(drv_data, MAX44009_REG_LUX_HIGH_BYTE, &val_h,
			      false) != 0) {
		return -EIO;
	}

	if (max44009_reg_read(drv_data, MAX44009_REG_LUX_LOW_BYTE, &val_l,
			      true) != 0) {
		return -EIO;
	}

	drv_data->sample = ((uint16_t)val_h) << 8;
	drv_data->sample += val_l;

	return 0;
}

static int max44009_channel_get(struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max44009_data *drv_data = dev->driver_data;
	uint32_t uval;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/**
	 * sample consists of 4 bits of exponent and 8 bits of mantissa
	 * bits 15 to 12 are exponent bits
	 * bits 11 to 8 and 3 to 0 are the mantissa bits
	 */
	uval = drv_data->sample;
	uval = (uval & MAX44009_MANTISSA_LOW_NIBBLE_MASK) +
	       ((uval & MAX44009_MANTISSA_HIGH_NIBBLE_MASK) >> 4);
	uval = uval << (drv_data->sample >> MAX44009_SAMPLE_EXPONENT_SHIFT);

	val->type = SENSOR_VALUE_TYPE_DOUBLE;
	/* lux is the integer of sample output multiplied by 0.045. */
	val->dval = ((double)uval) * 0.045;

	return 0;
}

static const struct sensor_driver_api max44009_driver_api = {
	.attr_set = max44009_attr_set,
	.sample_fetch = max44009_sample_fetch,
	.channel_get = max44009_channel_get,
};

int max44009_init(struct device *dev)
{
	struct max44009_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_MAX44009_I2C_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
			    CONFIG_MAX44009_I2C_DEV_NAME);
		return -EINVAL;
	}

	dev->driver_api = &max44009_driver_api;

	return 0;
}

static struct max44009_data max44009_drv_data;

DEVICE_INIT(max44009, CONFIG_MAX44009_DRV_NAME, max44009_init,
	    &max44009_drv_data, NULL, POST_KERNEL,
	    CONFIG_SENSOR_INIT_PRIORITY);
