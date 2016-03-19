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

#include "sensor_bma280.h"

static int bma280_reg_burst_read(struct bma280_data *drv_data,
				 uint8_t reg, uint8_t *buff,
				 int buff_len)
{
	struct i2c_msg msgs[2] = {
		{
			.buf = &reg,
			.len = 1,
			.flags = I2C_MSG_WRITE | I2C_MSG_RESTART,
		},
		{
			.buf = buff,
			.len = buff_len,
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		},
	};

	return i2c_transfer(drv_data->i2c, msgs, 2, BMA280_I2C_ADDRESS);
}

int bma280_reg_read(struct bma280_data *drv_data,
		    uint8_t reg, uint8_t *val)
{
	return bma280_reg_burst_read(drv_data, reg, val, 1);
}

int bma280_reg_write(struct bma280_data *drv_data,
		     uint8_t reg, uint8_t val)
{
	uint8_t tx_buf[2] = {reg, val};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 BMA280_I2C_ADDRESS);
}

int bma280_reg_update(struct bma280_data *drv_data,
		      uint8_t reg, uint8_t mask, uint8_t val)
{
	uint8_t old_val = 0;
	uint8_t new_val;

	if (bma280_reg_read(drv_data, reg, &old_val) != 0) {
		return -EIO;
	}

	new_val = old_val & ~mask;
	new_val |= val & mask;

	return bma280_reg_write(drv_data, reg, new_val);
}

static int bma280_sample_fetch(struct device *dev)
{
	struct bma280_data *drv_data = dev->driver_data;
	uint8_t buf[6];
	uint8_t lsb;
	int rc;

	/*
	 * since all accel data register addresses are consecutive,
	 * a burst read can be used to read all the samples
	 */
	rc = bma280_reg_burst_read(drv_data, BMA280_REG_ACCEL_X_LSB, buf, 6);
	if (rc != 0) {
		DBG("Could not read accel axis data\n");
		return -EIO;
	}

	lsb = (buf[0] & BMA280_ACCEL_LSB_MASK) >> BMA280_ACCEL_LSB_SHIFT;
	drv_data->x_sample = (((int8_t)buf[1]) << BMA280_ACCEL_LSB_BITS) + lsb;

	lsb = (buf[2] & BMA280_ACCEL_LSB_MASK) >> BMA280_ACCEL_LSB_SHIFT;
	drv_data->y_sample = (((int8_t)buf[3]) << BMA280_ACCEL_LSB_BITS) + lsb;

	lsb = (buf[4] & BMA280_ACCEL_LSB_MASK) >> BMA280_ACCEL_LSB_SHIFT;
	drv_data->z_sample = (((int8_t)buf[5]) << BMA280_ACCEL_LSB_BITS) + lsb;

	rc = bma280_reg_read(drv_data, BMA280_REG_TEMP, (uint8_t *)&drv_data->temp_sample);
	if (rc != 0) {
		DBG("Could not read temperature data\n");
		return -EIO;
	}

	return 0;
}

static int bma280_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bma280_data *drv_data = dev->driver_data;
	int64_t raw_val;

	/*
	 * See datasheet "Sensor data" section for
	 * more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_ACCEL_X) {
		raw_val = drv_data->x_sample;
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		raw_val = drv_data->y_sample;
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		raw_val = drv_data->z_sample;
	} else if (chan == SENSOR_CHAN_TEMP) {
		/* temperature_val = 23 + sample / 2 */
		val->type = SENSOR_TYPE_INT_PLUS_MICRO;
		val->val1 = (drv_data->temp_sample >> 1) + 23;
		val->val2 = 500000 * (drv_data->temp_sample & 1);
		return 0;
	} else {
		return -ENOTSUP;
	}

	/* accel_val = sample * BMA280_ACCEL_SCALE / 1000 */
	val->type = SENSOR_TYPE_INT_PLUS_MICRO;
	raw_val = raw_val * BMA280_ACCEL_SCALE;
	val->val1 = raw_val / 1000000000L;
	val->val2 = (raw_val % 1000000000L) / 1000;

	/* normalize val to make sure val->val2 is positive */
	if (val->val2 < 0) {
		val->val1 -= 1;
		val->val2 += 1000000;
	}

	return 0;
}

static struct sensor_driver_api bma280_driver_api = {
#if CONFIG_BMA280_TRIGGER
	.attr_set = bma280_attr_set,
	.trigger_set = bma280_trigger_set,
#endif
	.sample_fetch = bma280_sample_fetch,
	.channel_get = bma280_channel_get,
};

int bma280_init(struct device *dev)
{
	struct bma280_data *drv_data = dev->driver_data;
	uint8_t id = 0;
	int rc;

	dev->driver_api = &bma280_driver_api;

	drv_data->i2c = device_get_binding(CONFIG_BMA280_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		DBG("Could not get pointer to %s device\n",
		    CONFIG_BMA280_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	/* read device ID */
	rc = bma280_reg_read(drv_data, BMA280_REG_CHIP_ID, &id);
	if (rc != 0) {
		DBG("Could not read chip id\n");
		return -EIO;
	}

	if (id != BMA280_CHIP_ID) {
		DBG("Unexpected chip id (%x)\n", id);
		return -EIO;
	}

	/* set the data filter bandwidth */
	rc = bma280_reg_write(drv_data, BMA280_REG_PMU_BW,
			      BMA280_PMU_BW);
	if (rc != 0) {
		DBG("Could not set data filter bandwidth\n");
		return -EIO;
	}

	/* set g-range */
	rc = bma280_reg_write(drv_data, BMA280_REG_PMU_RANGE,
			      BMA280_PMU_RANGE);
	if (rc != 0) {
		DBG("Could not set data g-range\n");
		return -EIO;
	}

#ifdef CONFIG_BMA280_TRIGGER
	rc = bma280_init_interrupt(dev);
	if (rc != 0) {
		DBG("Could not initialize interrupts\n");
		return -EIO;
	}
#endif

	return 0;
}

struct bma280_data bma280_driver;

DEVICE_INIT(bma280, CONFIG_BMA280_NAME, bma280_init, &bma280_driver,
	    NULL, SECONDARY, CONFIG_BMA280_INIT_PRIORITY);
