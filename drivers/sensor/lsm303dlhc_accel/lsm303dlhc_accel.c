/*
 * Copyright (c) 2018 Philémon Jaermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <i2c.h>
#include <init.h>
#include <sensor.h>
#include <logging/sys_log.h>

#include "lsm303dlhc_accel.h"

static int lsm303dlhc_sample_fetch(struct device *dev,
				   enum sensor_channel chan)
{
	struct lsm303dlhc_accel_data *drv_data = dev->driver_data;
	u8_t accel_buf[6];

	if (i2c_burst_read(drv_data->i2c,
			   LSM303DLHC_ACCEL_I2C_ADDR,
			   LSM303DLHC_REG_ACCEL_X_LSB,
			   accel_buf, 6) < 0) {
		SYS_LOG_ERR("Could not read accel axis data.");
		return -EIO;
	}

	drv_data->accel_x = (accel_buf[1] << 8) | accel_buf[0];
	drv_data->accel_y = (accel_buf[3] << 8) | accel_buf[2];
	drv_data->accel_z = (accel_buf[5] << 8) | accel_buf[4];

	return 0;
}

static void lsm303dlhc_convert(struct sensor_value *val,
			       s64_t raw_val)
{
	s64_t val_mg = LSM303DLHC_ACCEL_SCALE * (raw_val >> 4);

	val->val1 = ((val_mg * SENSOR_G) / 1000) / 1000000LL;
	val->val2 = ((val_mg * SENSOR_G) / 1000) % 1000000LL;
}

static int lsm303dlhc_channel_get(struct device *dev,
				  enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct lsm303dlhc_accel_data *drv_data = dev->driver_data;

	if (chan == SENSOR_CHAN_ACCEL_X) {
		lsm303dlhc_convert(val, drv_data->accel_x);
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		lsm303dlhc_convert(val, drv_data->accel_y);
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		lsm303dlhc_convert(val, drv_data->accel_z);
	} else if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		lsm303dlhc_convert(val, drv_data->accel_x);
		lsm303dlhc_convert(val + 1, drv_data->accel_y);
		lsm303dlhc_convert(val + 2, drv_data->accel_z);
	} else {
		return -ENOTSUP;
	}
	return 0;
}

static const struct sensor_driver_api lsm303dlhc_accel_driver_api = {
	.sample_fetch = lsm303dlhc_sample_fetch,
	.channel_get = lsm303dlhc_channel_get,
};

static int lsm303dlhc_accel_init(struct device *dev)
{
	struct lsm303dlhc_accel_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(LSM303DLHC_ACCEL_I2C_MASTER);
	if (drv_data->i2c == NULL) {
		SYS_LOG_ERR("Could not get pointer to %s device",
				LSM303DLHC_ACCEL_I2C_MASTER);
		return -ENODEV;
	}

	/* Enable accel measurement and set power mode and data rate */
	if (i2c_reg_write_byte(drv_data->i2c,
			       LSM303DLHC_ACCEL_I2C_ADDR,
			       LSM303DLHC_REG_CTRL_1,
			       LSM303DLHC_ACCEL_EN_BITS |
			       LSM303DLHC_LP_EN_BIT |
			       LSM303DLHC_ACCEL_ODR_BITS) < 0) {
		SYS_LOG_ERR("Failed to configure chip.");
		return -EIO;
	}

	/* Set accelerometer full scale range */
	if (i2c_reg_write_byte(drv_data->i2c,
			       LSM303DLHC_ACCEL_I2C_ADDR,
			       LSM303DLHC_REG_CTRL_4,
			       LSM303DLHC_ACCEL_FS_BITS) < 0) {
		SYS_LOG_ERR("Failed to set accelerometer full scale range.");
		return -EIO;
	}

	return 0;
}

static struct lsm303dlhc_accel_data lsm303dlhc_accel_driver;

DEVICE_AND_API_INIT(lsm303dlhc_accel, CONFIG_LSM303DLHC_ACCEL_NAME,
		    lsm303dlhc_accel_init, &lsm303dlhc_accel_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &lsm303dlhc_accel_driver_api);
