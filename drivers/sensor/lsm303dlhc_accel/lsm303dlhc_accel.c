/*
 * Copyright (c) 2018 Philémon Jaermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <i2c.h>
#include <init.h>
#include <sensor.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(lsm303dlhc_accel);

#include "lsm303dlhc_accel.h"

static int lsm303dlhc_sample_fetch(struct device *dev,
				   enum sensor_channel chan)
{
	const struct lsm303dlhc_accel_config *config = dev->config->config_info;
	struct lsm303dlhc_accel_data *drv_data = dev->driver_data;
	u8_t accel_buf[6];

	if (i2c_burst_read(drv_data->i2c,
			   config->i2c_address,
			   LSM303DLHC_REG_ACCEL_X_LSB,
			   accel_buf, 6) < 0) {
		LOG_ERR("Could not read accel axis data.");
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

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lsm303dlhc_convert(val, drv_data->accel_x);
		break;
	case  SENSOR_CHAN_ACCEL_Y:
		lsm303dlhc_convert(val, drv_data->accel_y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lsm303dlhc_convert(val, drv_data->accel_z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm303dlhc_convert(val, drv_data->accel_x);
		lsm303dlhc_convert(val + 1, drv_data->accel_y);
		lsm303dlhc_convert(val + 2, drv_data->accel_z);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int lsm303dlhc_set_sampling_frequency(struct device *dev,
					     const struct sensor_value *val)
{
	struct lsm303dlhc_accel_data *drv_data = dev->driver_data;
	const struct lsm303dlhc_accel_config *config = dev->config->config_info;
	u8_t freq_value;
	int status;

	switch (val->val1) {
	case 1:
		freq_value = LSM303DLHC_ACCEL_ODR_1HZ;
		break;
	case 10:
		freq_value = LSM303DLHC_ACCEL_ODR_10HZ;
		break;
	case 25:
		freq_value = LSM303DLHC_ACCEL_ODR_25HZ;
		break;
	case 50:
		freq_value = LSM303DLHC_ACCEL_ODR_50HZ;
		break;
	case 100:
		freq_value = LSM303DLHC_ACCEL_ODR_100HZ;
		break;
	case 200:
		freq_value = LSM303DLHC_ACCEL_ODR_200HZ;
		break;
	case 400:
		freq_value = LSM303DLHC_ACCEL_ODR_400HZ;
		break;
#if CONFIG_LSM303DLHC_ACCEL_POWER_MODE == 0
	case 1620:
		freq_value = LSM303DLHC_ACCEL_ODR_1620HZ;
		break;
#else
	case 1344:
		freq_value = LSM303DLHC_ACCEL_ODR_1344HZ;
		break;
	case 5376:
		freq_value = LSM303DLHC_ACCEL_ODR_5376HZ;
		break;
#endif
	default:
		return -ENOTSUP;
	}

	status = i2c_reg_update_byte(drv_data->i2c, config->i2c_address,
				     LSM303DLHC_REG_CTRL_1,
				     LSM303DLHC_ACCEL_ODR_MASK,
				     freq_value << LSM303DLHC_ACCEL_ODR_SHIFT);
	if (status < 0) {
		LOG_ERR("Could not update sampling frequency.");
		return -EIO;
	}

	return 0;
}


static int lsm303dlhc_attr_set(struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm303dlhc_set_sampling_frequency(dev, val);
	default:
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api lsm303dlhc_accel_driver_api = {
	.sample_fetch = lsm303dlhc_sample_fetch,
	.channel_get = lsm303dlhc_channel_get,
	.attr_set = lsm303dlhc_attr_set,
};

static int lsm303dlhc_accel_init(struct device *dev)
{
	const struct lsm303dlhc_accel_config *config = dev->config->config_info;
	struct lsm303dlhc_accel_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(config->i2c_name);
	if (drv_data->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device",
			    config->i2c_name);
		return -ENODEV;
	}

	/* Enable accel measurement and set power mode and data rate */
	if (i2c_reg_write_byte(drv_data->i2c,
			       config->i2c_address,
			       LSM303DLHC_REG_CTRL_1,
			       LSM303DLHC_ACCEL_EN_BITS |
			       LSM303DLHC_LP_EN_BIT |
			       LSM303DLHC_ACCEL_ODR_BITS) < 0) {
		LOG_ERR("Failed to configure chip.");
		return -EIO;
	}

	/* Set accelerometer full scale range */
	if (i2c_reg_write_byte(drv_data->i2c,
			       config->i2c_address,
			       LSM303DLHC_REG_CTRL_4,
			       LSM303DLHC_ACCEL_FS_BITS) < 0) {
		LOG_ERR("Failed to set accelerometer full scale range.");
		return -EIO;
	}

	return 0;
}

static const struct lsm303dlhc_accel_config lsm303dlhc_accel_config = {
	.i2c_name = DT_ST_LSM303DLHC_ACCEL_0_BUS_NAME,
	.i2c_address = DT_ST_LSM303DLHC_ACCEL_0_BASE_ADDRESS,
};

static struct lsm303dlhc_accel_data lsm303dlhc_accel_driver;

DEVICE_AND_API_INIT(lsm303dlhc_accel, DT_ST_LSM303DLHC_ACCEL_0_LABEL,
		    lsm303dlhc_accel_init, &lsm303dlhc_accel_driver,
		    &lsm303dlhc_accel_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lsm303dlhc_accel_driver_api);
