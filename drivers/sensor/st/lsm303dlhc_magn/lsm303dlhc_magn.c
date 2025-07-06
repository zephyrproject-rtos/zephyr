/*
 * Copyright (c) 2018 Philémon Jaermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm303dlhc_magn

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lsm303dlhc_magn, CONFIG_SENSOR_LOG_LEVEL);

#include "lsm303dlhc_magn.h"

static int lsm303dlhc_sample_fetch(const struct device *dev,
				   enum sensor_channel chan)
{
	const struct lsm303dlhc_magn_config *config = dev->config;
	struct lsm303dlhc_magn_data *drv_data = dev->data;
	uint8_t magn_buf[6];
	uint8_t status;

	/* Check data ready flag */
	if (i2c_reg_read_byte_dt(&config->i2c, LSM303DLHC_SR_REG_M,
				 &status) < 0) {
		LOG_ERR("Failed to read status register.");
		return -EIO;
	}

	if (!(status & LSM303DLHC_MAGN_DRDY)) {
		LOG_ERR("Sensor data not available.");
		return -EIO;
	}

	if (i2c_burst_read_dt(&config->i2c, LSM303DLHC_REG_MAGN_X_LSB,
			      magn_buf, 6) < 0) {
		LOG_ERR("Could not read magn axis data.");
		return -EIO;
	}

	drv_data->magn_x = (magn_buf[0] << 8) | magn_buf[1];
	drv_data->magn_y = (magn_buf[4] << 8) | magn_buf[5];
	drv_data->magn_z = (magn_buf[2] << 8) | magn_buf[3];

	return 0;
}

static void lsm303dlhc_convert_xy(struct sensor_value *val,
			       int64_t raw_val)
{
	val->val1 = raw_val / LSM303DLHC_MAGN_LSB_GAUSS_XY;
	val->val2 = (1000000 * raw_val / LSM303DLHC_MAGN_LSB_GAUSS_XY) % 1000000;
}

static void lsm303dlhc_convert_z(struct sensor_value *val,
			       int64_t raw_val)
{
	val->val1 = raw_val / LSM303DLHC_MAGN_LSB_GAUSS_Z;
	val->val2 = (1000000 * raw_val / LSM303DLHC_MAGN_LSB_GAUSS_Z) % 1000000;
}

static int lsm303dlhc_channel_get(const struct device *dev,
				  enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct lsm303dlhc_magn_data *drv_data = dev->data;

	switch (chan) {
	case  SENSOR_CHAN_MAGN_X:
		lsm303dlhc_convert_xy(val, drv_data->magn_x);
		break;
	case SENSOR_CHAN_MAGN_Y:
		lsm303dlhc_convert_xy(val, drv_data->magn_y);
		break;
	case SENSOR_CHAN_MAGN_Z:
		lsm303dlhc_convert_z(val, drv_data->magn_z);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		lsm303dlhc_convert_xy(val, drv_data->magn_x);
		lsm303dlhc_convert_xy(val + 1, drv_data->magn_y);
		lsm303dlhc_convert_z(val + 2, drv_data->magn_z);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static DEVICE_API(sensor, lsm303dlhc_magn_driver_api) = {
	.sample_fetch = lsm303dlhc_sample_fetch,
	.channel_get = lsm303dlhc_channel_get,
};

static int lsm303dlhc_magn_init(const struct device *dev)
{
	const struct lsm303dlhc_magn_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* Set magnetometer output data rate */
	if (i2c_reg_write_byte_dt(&config->i2c, LSM303DLHC_CRA_REG_M,
				  LSM303DLHC_MAGN_ODR_BITS) < 0) {
		LOG_ERR("Failed to configure chip.");
		return -EIO;
	}

	/* Set magnetometer full scale range */
	if (i2c_reg_write_byte_dt(&config->i2c, LSM303DLHC_CRB_REG_M,
				  LSM303DLHC_MAGN_FS_BITS) < 0) {
		LOG_ERR("Failed to set magnetometer full scale range.");
		return -EIO;
	}

	/* Continuous update */
	if (i2c_reg_write_byte_dt(&config->i2c, LSM303DLHC_MR_REG_M,
				  LSM303DLHC_MAGN_CONT_UPDATE) < 0) {
		LOG_ERR("Failed to enable continuous data update.");
		return -EIO;
	}
	return 0;
}

#define LSM303DLHC_MAGN_DEFINE(inst)								\
	static struct lsm303dlhc_magn_data lsm303dlhc_magn_data_##inst;				\
												\
	static const struct lsm303dlhc_magn_config lsm303dlhc_magn_config_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lsm303dlhc_magn_init, NULL,				\
			      &lsm303dlhc_magn_data_##inst, &lsm303dlhc_magn_config_##inst,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,				\
			      &lsm303dlhc_magn_driver_api);					\

DT_INST_FOREACH_STATUS_OKAY(LSM303DLHC_MAGN_DEFINE)
