/* lsm6ds0.c - Driver for LSM6DS0 accelerometer, gyroscope and
 * temperature sensor
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm6ds0

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "lsm6ds0.h"

LOG_MODULE_REGISTER(LSM6DS0, CONFIG_SENSOR_LOG_LEVEL);

static inline int lsm6ds0_reboot(const struct device *dev)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM6DS0_REG_CTRL_REG8,
				LSM6DS0_MASK_CTRL_REG8_BOOT,
				1 << LSM6DS0_SHIFT_CTRL_REG8_BOOT) < 0) {
		return -EIO;
	}

	k_busy_wait(USEC_PER_MSEC * 50U);

	return 0;
}

static inline int lsm6ds0_accel_axis_ctrl(const struct device *dev, int x_en,
					  int y_en, int z_en)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;
	uint8_t state = (x_en << LSM6DS0_SHIFT_CTRL_REG5_XL_XEN_XL) |
			(y_en << LSM6DS0_SHIFT_CTRL_REG5_XL_YEN_XL) |
			(z_en << LSM6DS0_SHIFT_CTRL_REG5_XL_ZEN_XL);

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LSM6DS0_REG_CTRL_REG5_XL,
				   LSM6DS0_MASK_CTRL_REG5_XL_XEN_XL |
				   LSM6DS0_MASK_CTRL_REG5_XL_YEN_XL |
				   LSM6DS0_MASK_CTRL_REG5_XL_ZEN_XL,
				   state);
}

static int lsm6ds0_accel_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM6DS0_REG_CTRL_REG6_XL,
				LSM6DS0_MASK_CTRL_REG6_XL_FS_XL,
				fs << LSM6DS0_SHIFT_CTRL_REG6_XL_FS_XL) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6ds0_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM6DS0_REG_CTRL_REG6_XL,
				LSM6DS0_MASK_CTRL_REG6_XL_ODR_XL,
				odr << LSM6DS0_SHIFT_CTRL_REG6_XL_ODR_XL) < 0) {
		return -EIO;
	}

	return 0;
}

static inline int lsm6ds0_gyro_axis_ctrl(const struct device *dev, int x_en,
					 int y_en,
					 int z_en)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;
	uint8_t state = (x_en << LSM6DS0_SHIFT_CTRL_REG4_XEN_G) |
			(y_en << LSM6DS0_SHIFT_CTRL_REG4_YEN_G) |
			(z_en << LSM6DS0_SHIFT_CTRL_REG4_ZEN_G);

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LSM6DS0_REG_CTRL_REG4,
				   LSM6DS0_MASK_CTRL_REG4_XEN_G |
				   LSM6DS0_MASK_CTRL_REG4_YEN_G |
				   LSM6DS0_MASK_CTRL_REG4_ZEN_G,
				   state);
}

static int lsm6ds0_gyro_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM6DS0_REG_CTRL_REG1_G,
				LSM6DS0_MASK_CTRL_REG1_G_FS_G,
				fs << LSM6DS0_SHIFT_CTRL_REG1_G_FS_G) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6ds0_gyro_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM6DS0_REG_CTRL_REG1_G,
				LSM6DS0_MASK_CTRL_REG1_G_ODR_G,
				odr << LSM6DS0_SHIFT_CTRL_REG1_G_ODR_G) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6ds0_sample_fetch_accel(const struct device *dev)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;
	uint8_t buf[6];

	if (i2c_burst_read(data->i2c_master, config->i2c_slave_addr,
			   LSM6DS0_REG_OUT_X_L_XL, buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

#if defined(CONFIG_LSM6DS0_ACCEL_ENABLE_X_AXIS)
	data->accel_sample_x = (int16_t)((uint16_t)(buf[0]) |
				((uint16_t)(buf[1]) << 8));
#endif
#if defined(CONFIG_LSM6DS0_ACCEL_ENABLE_Y_AXIS)
	data->accel_sample_y = (int16_t)((uint16_t)(buf[2]) |
				((uint16_t)(buf[3]) << 8));
#endif
#if defined(CONFIG_LSM6DS0_ACCEL_ENABLE_Z_AXIS)
	data->accel_sample_z = (int16_t)((uint16_t)(buf[4]) |
				((uint16_t)(buf[5]) << 8));
#endif

	return 0;
}

static int lsm6ds0_sample_fetch_gyro(const struct device *dev)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;
	uint8_t buf[6];

	if (i2c_burst_read(data->i2c_master, config->i2c_slave_addr,
			   LSM6DS0_REG_OUT_X_L_G, buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

#if defined(CONFIG_LSM6DS0_GYRO_ENABLE_X_AXIS)
	data->gyro_sample_x = (int16_t)((uint16_t)(buf[0]) |
				((uint16_t)(buf[1]) << 8));
#endif
#if defined(CONFIG_LSM6DS0_GYRO_ENABLE_Y_AXIS)
	data->gyro_sample_y = (int16_t)((uint16_t)(buf[2]) |
				((uint16_t)(buf[3]) << 8));
#endif
#if defined(CONFIG_LSM6DS0_GYRO_ENABLE_Z_AXIS)
	data->gyro_sample_z = (int16_t)((uint16_t)(buf[4]) |
				((uint16_t)(buf[5]) << 8));
#endif

	return 0;
}

#if defined(CONFIG_LSM6DS0_ENABLE_TEMP)
static int lsm6ds0_sample_fetch_temp(const struct device *dev)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;
	uint8_t buf[2];

	if (i2c_burst_read(data->i2c_master, config->i2c_slave_addr,
			   LSM6DS0_REG_OUT_TEMP_L, buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->temp_sample = (int16_t)((uint16_t)(buf[0]) |
				((uint16_t)(buf[1]) << 8));

	return 0;
}
#endif

static int lsm6ds0_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_ACCEL_XYZ ||
#if defined(CONFIG_LSM6DS0_ENABLE_TEMP)
			chan == SENSOR_CHAN_DIE_TEMP ||
#endif
			chan == SENSOR_CHAN_GYRO_XYZ);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6ds0_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6ds0_sample_fetch_gyro(dev);
		break;
#if defined(CONFIG_LSM6DS0_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm6ds0_sample_fetch_temp(dev);
		break;
#endif
	case SENSOR_CHAN_ALL:
		lsm6ds0_sample_fetch_accel(dev);
		lsm6ds0_sample_fetch_gyro(dev);
#if defined(CONFIG_LSM6DS0_ENABLE_TEMP)
		lsm6ds0_sample_fetch_temp(dev);
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lsm6ds0_accel_convert(struct sensor_value *val, int raw_val,
					 float scale)
{
	double dval;

	dval = (double)(raw_val) * scale / 32767.0;
	val->val1 = (int32_t)dval;
	val->val2 = ((int32_t)(dval * 1000000)) % 1000000;
}

static inline int lsm6ds0_accel_get_channel(enum sensor_channel chan,
					    struct sensor_value *val,
					    struct lsm6ds0_data *data,
					    float scale)
{
	switch (chan) {
#if defined(CONFIG_LSM6DS0_ACCEL_ENABLE_X_AXIS)
	case SENSOR_CHAN_ACCEL_X:
		lsm6ds0_accel_convert(val, data->accel_sample_x, scale);
		break;
#endif
#if defined(CONFIG_LSM6DS0_ACCEL_ENABLE_Y_AXIS)
	case SENSOR_CHAN_ACCEL_Y:
		lsm6ds0_accel_convert(val, data->accel_sample_y, scale);
		break;
#endif
#if defined(CONFIG_LSM6DS0_ACCEL_ENABLE_Z_AXIS)
	case SENSOR_CHAN_ACCEL_Z:
		lsm6ds0_accel_convert(val, data->accel_sample_z, scale);
		break;
#endif
	case SENSOR_CHAN_ACCEL_XYZ:
#if defined(CONFIG_LSM6DS0_ACCEL_ENABLE_X_AXIS)
		lsm6ds0_accel_convert(val, data->accel_sample_x, scale);
#endif
#if defined(CONFIG_LSM6DS0_ACCEL_ENABLE_Y_AXIS)
		lsm6ds0_accel_convert(val + 1, data->accel_sample_y, scale);
#endif
#if defined(CONFIG_LSM6DS0_ACCEL_ENABLE_Z_AXIS)
		lsm6ds0_accel_convert(val + 2, data->accel_sample_z, scale);
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6ds0_accel_channel_get(enum sensor_channel chan,
				     struct sensor_value *val,
				     struct lsm6ds0_data *data)
{
	return lsm6ds0_accel_get_channel(chan, val, data,
					LSM6DS0_DEFAULT_ACCEL_FULLSCALE_FACTOR);
}

static inline void lsm6ds0_gyro_convert(struct sensor_value *val, int raw_val,
					float numerator)
{
	double dval;

	dval = (double)(raw_val) * numerator / 1000.0 * SENSOR_DEG2RAD_DOUBLE;
	val->val1 = (int32_t)dval;
	val->val2 = ((int32_t)(dval * 1000000)) % 1000000;
}

static inline int lsm6ds0_gyro_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm6ds0_data *data,
					   float numerator)
{
	switch (chan) {
#if defined(CONFIG_LSM6DS0_GYRO_ENABLE_X_AXIS)
	case SENSOR_CHAN_GYRO_X:
		lsm6ds0_gyro_convert(val, data->gyro_sample_x, numerator);
		break;
#endif
#if defined(CONFIG_LSM6DS0_GYRO_ENABLE_Y_AXIS)
	case SENSOR_CHAN_GYRO_Y:
		lsm6ds0_gyro_convert(val, data->gyro_sample_y, numerator);
		break;
#endif
#if defined(CONFIG_LSM6DS0_GYRO_ENABLE_Z_AXIS)
	case SENSOR_CHAN_GYRO_Z:
		lsm6ds0_gyro_convert(val, data->gyro_sample_z, numerator);
		break;
#endif
	case SENSOR_CHAN_GYRO_XYZ:
#if defined(CONFIG_LSM6DS0_GYRO_ENABLE_X_AXIS)
		lsm6ds0_gyro_convert(val, data->gyro_sample_x, numerator);
#endif
#if defined(CONFIG_LSM6DS0_GYRO_ENABLE_Y_AXIS)
		lsm6ds0_gyro_convert(val + 1, data->gyro_sample_y, numerator);
#endif
#if defined(CONFIG_LSM6DS0_GYRO_ENABLE_Z_AXIS)
		lsm6ds0_gyro_convert(val + 2, data->gyro_sample_z, numerator);
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6ds0_gyro_channel_get(enum sensor_channel chan,
				    struct sensor_value *val,
				    struct lsm6ds0_data *data)
{
	return lsm6ds0_gyro_get_channel(chan, val, data,
					LSM6DS0_DEFAULT_GYRO_FULLSCALE_FACTOR);
}

#if defined(CONFIG_LSM6DS0_ENABLE_TEMP)
static void lsm6ds0_gyro_channel_get_temp(struct sensor_value *val,
					  struct lsm6ds0_data *data)
{
	/* val = temp_sample / 16 + 25 */
	val->val1 = data->temp_sample / 16 + 25;
	val->val2 = (data->temp_sample % 16) * (1000000 / 16);
}
#endif

static int lsm6ds0_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lsm6ds0_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6ds0_accel_channel_get(chan, val, data);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6ds0_gyro_channel_get(chan, val, data);
		break;
#if defined(CONFIG_LSM6DS0_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm6ds0_gyro_channel_get_temp(val, data);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lsm6ds0_api_funcs = {
	.sample_fetch = lsm6ds0_sample_fetch,
	.channel_get = lsm6ds0_channel_get,
};

static int lsm6ds0_init_chip(const struct device *dev)
{
	struct lsm6ds0_data *data = dev->data;
	const struct lsm6ds0_config *config = dev->config;
	uint8_t chip_id;

	if (lsm6ds0_reboot(dev) < 0) {
		LOG_DBG("failed to reboot device");
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LSM6DS0_REG_WHO_AM_I, &chip_id) < 0) {
		LOG_DBG("failed reading chip id");
		return -EIO;
	}
	if (chip_id != LSM6DS0_VAL_WHO_AM_I) {
		LOG_DBG("invalid chip id 0x%x", chip_id);
		return -EIO;
	}
	LOG_DBG("chip id 0x%x", chip_id);

	if (lsm6ds0_accel_axis_ctrl(dev, LSM6DS0_ACCEL_ENABLE_X_AXIS,
				    LSM6DS0_ACCEL_ENABLE_Y_AXIS,
				    LSM6DS0_ACCEL_ENABLE_Z_AXIS) < 0) {
		LOG_DBG("failed to set accelerometer axis");
		return -EIO;
	}

	if (lsm6ds0_accel_set_fs_raw(dev, LSM6DS0_DEFAULT_ACCEL_FULLSCALE)
				     < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	if (lsm6ds0_accel_set_odr_raw(dev, LSM6DS0_DEFAULT_ACCEL_SAMPLING_RATE)
				      < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	if (lsm6ds0_gyro_axis_ctrl(dev, LSM6DS0_GYRO_ENABLE_X_AXIS,
				   LSM6DS0_GYRO_ENABLE_Y_AXIS,
				   LSM6DS0_GYRO_ENABLE_Z_AXIS) < 0) {
		LOG_DBG("failed to set gyroscope axis");
		return -EIO;
	}

	if (lsm6ds0_gyro_set_fs_raw(dev, LSM6DS0_DEFAULT_GYRO_FULLSCALE)
				    < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	if (lsm6ds0_gyro_set_odr_raw(dev, LSM6DS0_DEFAULT_GYRO_SAMPLING_RATE)
				     < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LSM6DS0_REG_CTRL_REG8,
				LSM6DS0_MASK_CTRL_REG8_BDU |
				LSM6DS0_MASK_CTRL_REG8_BLE |
				LSM6DS0_MASK_CTRL_REG8_IF_ADD_INC,
				(1 << LSM6DS0_SHIFT_CTRL_REG8_BDU) |
				(0 << LSM6DS0_SHIFT_CTRL_REG8_BLE) |
				(1 << LSM6DS0_SHIFT_CTRL_REG8_IF_ADD_INC))
				< 0) {
		LOG_DBG("failed to set BDU, BLE and burst");
		return -EIO;
	}

	return 0;
}

static int lsm6ds0_init(const struct device *dev)
{
	const struct lsm6ds0_config * const config = dev->config;
	struct lsm6ds0_data *data = dev->data;

	data->i2c_master = device_get_binding(config->i2c_master_dev_name);
	if (!data->i2c_master) {
		LOG_DBG("i2c master not found: %s",
			    config->i2c_master_dev_name);
		return -EINVAL;
	}

	if (lsm6ds0_init_chip(dev) < 0) {
		LOG_DBG("failed to initialize chip");
		return -EIO;
	}

	return 0;
}

static const struct lsm6ds0_config lsm6ds0_config = {
	.i2c_master_dev_name = DT_INST_BUS_LABEL(0),
	.i2c_slave_addr = DT_INST_REG_ADDR(0),
};

static struct lsm6ds0_data lsm6ds0_data;

DEVICE_AND_API_INIT(lsm6ds0, DT_INST_LABEL(0), lsm6ds0_init,
		    &lsm6ds0_data, &lsm6ds0_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lsm6ds0_api_funcs);
