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

#include <i2c.h>
#include <init.h>
#include <sensor.h>

#include "sensor_lis3dh.h"

static int lis3dh_i2c_read(struct lis3dh_data *drv_data,
			   uint8_t cmd, uint8_t *buff,
			   int buff_len)
{
	struct i2c_msg msgs[2] = {
		{
			.buf = &cmd,
			.len = 1,
			.flags = I2C_MSG_WRITE | I2C_MSG_RESTART,
		},
		{
			.buf = buff,
			.len = buff_len,
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		},
	};

	return i2c_transfer(drv_data->i2c, msgs, 2, LIS3DH_I2C_ADDRESS);
}

static inline int lis3dh_reg_burst_read(struct lis3dh_data *drv_data,
				 uint8_t reg, uint8_t *buff,
				 int buff_len)
{
	reg = reg | LIS3DH_AUTOINCREMENT_ADDR;
	return lis3dh_i2c_read(drv_data, reg, buff, buff_len);
}

static inline int lis3dh_reg_read(struct lis3dh_data *drv_data,
		    uint8_t reg, uint8_t *val)
{
	return lis3dh_reg_burst_read(drv_data, reg, val, 1);
}

int lis3dh_reg_write(struct lis3dh_data *drv_data,
		     uint8_t reg, uint8_t val)
{
	uint8_t tx_buf[2] = {reg, val};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 LIS3DH_I2C_ADDRESS);
}

static int lis3dh_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct lis3dh_data *drv_data = dev->driver_data;
	int64_t raw_val;

	if (chan == SENSOR_CHAN_ACCEL_X) {
		raw_val = drv_data->x_sample;
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		raw_val = drv_data->y_sample;
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		raw_val = drv_data->z_sample;
	} else {
		return -ENOTSUP;
	}

	/* val = raw_val * LIS3DH_ACCEL_SCALE / (1000 * (2^16 - 1)) */
	val->type = SENSOR_TYPE_INT_PLUS_MICRO;
	raw_val = raw_val * LIS3DH_ACCEL_SCALE;
	val->val1 = raw_val / (1000 * 0xFFFF);
	val->val2 = (raw_val % (1000 * 0xFFFF)) * 1000000 / (1000 * 0xFFFF);

	/* normalize val to make sure val->val2 is positive */
	if (val->val2 < 0) {
		val->val1 -= 1;
		val->val2 += 1000000;
	}

	return 0;
}

int lis3dh_sample_fetch(struct device *dev)
{
	struct lis3dh_data *drv_data = dev->driver_data;
	uint8_t buf[6];
	int rc;

	/*
	 * since all accel data register addresses are consecutive,
	 * a burst read can be used to read all the samples
	 */
	rc = lis3dh_reg_burst_read(drv_data, LIS3DH_REG_ACCEL_X_LSB, buf, 6);
	if (rc != 0) {
		DBG("Could not read accel axis data\n");
		return -EIO;
	}

	drv_data->x_sample = (buf[1] << 8) | buf[0];
	drv_data->y_sample = (buf[3] << 8) | buf[2];
	drv_data->z_sample = (buf[5] << 8) | buf[4];

	return 0;
}

static struct sensor_driver_api lis3dh_driver_api = {
#if CONFIG_LIS3DH_TRIGGER
	.trigger_set = lis3dh_trigger_set,
#endif
	.sample_fetch = lis3dh_sample_fetch,
	.channel_get = lis3dh_channel_get,
};

int lis3dh_init(struct device *dev)
{
	struct lis3dh_data *drv_data = dev->driver_data;
	int rc;

	dev->driver_api = &lis3dh_driver_api;

	drv_data->i2c = device_get_binding(CONFIG_LIS3DH_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		DBG("Could not get pointer to %s device\n",
		    CONFIG_LIS3DH_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	/* enable accel measurements and set power mode and data rate */
	rc = lis3dh_reg_write(drv_data, LIS3DH_REG_CTRL1, LIS3DH_ACCEL_EN_BITS |
			      LIS3DH_LP_EN_BIT | LIS3DH_ODR_BITS);
	if (rc != 0) {
		DBG("Failed to configure chip.\n");
	}

	/* set full scale range */
	rc = lis3dh_reg_write(drv_data, LIS3DH_REG_CTRL4, LIS3DH_FS_BITS);
	if (rc != 0) {
		DBG("Failed to set full scale range.\n");
		return -EIO;
	}

#ifdef CONFIG_LIS3DH_TRIGGER
	rc = lis3dh_init_interrupt(dev);
	if (rc != 0) {
		DBG("Failed to initialize interrupts.\n");
		return -EIO;
	}
#endif

	return 0;
}

struct lis3dh_data lis3dh_driver;

DEVICE_INIT(lis3dh, CONFIG_LIS3DH_NAME, lis3dh_init, &lis3dh_driver,
	    NULL, SECONDARY, CONFIG_LIS3DH_INIT_PRIORITY);
