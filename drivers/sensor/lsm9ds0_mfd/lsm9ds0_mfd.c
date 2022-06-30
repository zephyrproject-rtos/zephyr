/* sensor_lsm9ds0_mfd.c - Driver for LSM9DS0 accelerometer, magnetometer
 * and temperature (MFD) sensor driver
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm9ds0_mfd

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lsm9ds0_mfd.h"

LOG_MODULE_REGISTER(LSM9DS0_MFD, CONFIG_SENSOR_LOG_LEVEL);

static inline int lsm9ds0_mfd_reboot_memory(const struct device *dev)
{
	const struct lsm9ds0_mfd_config *config = dev->config;

	if (i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG0_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG0_XM_BOOT,
				   1 << LSM9DS0_MFD_SHIFT_CTRL_REG0_XM_BOOT) < 0) {
		return -EIO;
	}

	k_busy_wait(USEC_PER_MSEC * 50U);

	return 0;
}

#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
static inline int lsm9ds0_mfd_accel_set_odr_raw(const struct device *dev,
						uint8_t odr)
{
	const struct lsm9ds0_mfd_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG1_XM,
				      LSM9DS0_MFD_MASK_CTRL_REG1_XM_AODR,
				      odr << LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AODR);
}

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_SAMPLING_RATE_RUNTIME)
static const struct {
	int freq_int;
	int freq_micro;
} lsm9ds0_mfd_accel_odr_map[] = { {0, 0},
				  {3, 125000},
				  {6, 250000},
				  {12, 500000},
				  {25, 0},
				  {50, 0},
				  {100, 0},
				  {200, 0},
				  {400, 0},
				  {800, 0},
				  {1600, 0} };

static int lsm9ds0_mfd_accel_set_odr(const struct device *dev,
				     const struct sensor_value *val)
{
	uint8_t i;

	for (i = 0U; i < ARRAY_SIZE(lsm9ds0_mfd_accel_odr_map); ++i) {
		if (val->val1 < lsm9ds0_mfd_accel_odr_map[i].freq_int ||
		    (val->val1 == lsm9ds0_mfd_accel_odr_map[i].freq_int &&
		     val->val2 <= lsm9ds0_mfd_accel_odr_map[i].freq_micro)) {
			return lsm9ds0_mfd_accel_set_odr_raw(dev, i);
		}
	}

	return -ENOTSUP;
}
#endif

static inline int lsm9ds0_mfd_accel_set_fs_raw(const struct device *dev,
					       uint8_t fs)
{
	const struct lsm9ds0_mfd_config *config = dev->config;

	if (i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG2_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG2_XM_AFS,
				   fs << LSM9DS0_MFD_SHIFT_CTRL_REG2_XM_AFS) < 0) {
		return -EIO;
	}

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
	data->accel_fs = fs;
#endif

	return 0;
}

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
static const struct {
	int fs;
} lsm9ds0_mfd_accel_fs_map[] = { {2},
				 {4},
				 {6},
				 {8},
				 {16} };

static int lsm9ds0_mfd_accel_set_fs(const struct device *dev, int val)
{
	uint8_t i;

	for (i = 0U; i < ARRAY_SIZE(lsm9ds0_mfd_accel_fs_map); ++i) {
		if (val <= lsm9ds0_mfd_accel_fs_map[i].fs) {
			return lsm9ds0_mfd_accel_set_fs_raw(dev, i);
		}
	}

	return -ENOTSUP;
}
#endif
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
static inline int lsm9ds0_mfd_magn_set_odr_raw(const struct device *dev,
					       uint8_t odr)
{
	const struct lsm9ds0_mfd_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG5_XM,
				      LSM9DS0_MFD_MASK_CTRL_REG5_XM_M_ODR,
				      odr << LSM9DS0_MFD_SHIFT_CTRL_REG5_XM_M_ODR);
}

#if defined(CONFIG_LSM9DS0_MFD_MAGN_SAMPLING_RATE_RUNTIME)
static const struct {
	int freq_int;
	int freq_micro;
} lsm9ds0_mfd_magn_odr_map[] = { {0, 0},
				 {3, 125000},
				 {6, 250000},
				 {12, 500000},
				 {25, 0},
				 {50, 0},
				 {100, 0} };

static int lsm9ds0_mfd_magn_set_odr(const struct device *dev,
				    const struct sensor_value *val)
{
	uint8_t i;

	for (i = 0U; i < ARRAY_SIZE(lsm9ds0_mfd_accel_odr_map); ++i) {
		if (val->val1 < lsm9ds0_mfd_accel_odr_map[i].freq_int ||
		    (val->val1 == lsm9ds0_mfd_accel_odr_map[i].freq_int &&
		     val->val2 <= lsm9ds0_mfd_accel_odr_map[i].freq_micro)) {
			return lsm9ds0_mfd_magn_set_odr_raw(dev, i);
		}
	}

	return -ENOTSUP;
}
#endif

static inline int lsm9ds0_mfd_magn_set_fs_raw(const struct device *dev,
					      uint8_t fs)
{
	const struct lsm9ds0_mfd_config *config = dev->config;

	if (i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG6_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG6_XM_MFS,
				   fs << LSM9DS0_MFD_SHIFT_CTRL_REG6_XM_MFS) < 0) {
		return -EIO;
	}

#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
	data->magn_fs = fs;
#endif

	return 0;
}

#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
static const struct {
	int fs;
} lsm9ds0_mfd_magn_fs_map[] = { {2},
				{4},
				{8},
				{12} };

static int lsm9ds0_mfd_magn_set_fs(const struct device *dev,
				   const struct sensor_value *val)
{
	uint8_t i;

	for (i = 0U; i < ARRAY_SIZE(lsm9ds0_mfd_magn_fs_map); ++i) {
		if (val->val1 <= lsm9ds0_mfd_magn_fs_map[i].fs) {
			return lsm9ds0_mfd_magn_set_fs_raw(dev, i);
		}
	}

	return -ENOTSUP;
}
#endif
#endif

#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
static inline int lsm9ds0_mfd_sample_fetch_accel(const struct device *dev)
{
	struct lsm9ds0_mfd_data *data = dev->data;
	const struct lsm9ds0_mfd_config *config = dev->config;
	uint8_t out_l, out_h;

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_ENABLE_X)
	if (i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_X_L_A, &out_l) < 0 ||
	    i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_X_H_A, &out_h) < 0) {
		LOG_DBG("failed to read accel sample (X axis)");
		return -EIO;
	}

	data->sample_accel_x = (int16_t)((uint16_t)(out_l) |
			       ((uint16_t)(out_h) << 8));
#endif

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_ENABLE_Y)
	if (i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_Y_L_A, &out_l) < 0 ||
	    i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_Y_H_A, &out_h) < 0) {
		LOG_DBG("failed to read accel sample (Y axis)");
		return -EIO;
	}

	data->sample_accel_y = (int16_t)((uint16_t)(out_l) |
			       ((uint16_t)(out_h) << 8));
#endif

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_ENABLE_Z)
	if (i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_Z_L_A, &out_l) < 0 ||
	    i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_Z_H_A, &out_h) < 0) {
		LOG_DBG("failed to read accel sample (Z axis)");
		return -EIO;
	}

	data->sample_accel_z = (int16_t)((uint16_t)(out_l) |
			       ((uint16_t)(out_h) << 8));
#endif

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
	data->sample_accel_fs = data->accel_fs;
#endif

	return 0;
}
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
static inline int lsm9ds0_mfd_sample_fetch_magn(const struct device *dev)
{
	struct lsm9ds0_mfd_data *data = dev->data;
	const struct lsm9ds0_mfd_config *config = dev->config;
	uint8_t out_l, out_h;

	if (i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_X_L_M, &out_l) < 0 ||
	    i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_X_H_M, &out_h) < 0) {
		LOG_DBG("failed to read magn sample (X axis)");
		return -EIO;
	}

	data->sample_magn_x = (int16_t)((uint16_t)(out_l) |
			      ((uint16_t)(out_h) << 8));

	if (i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_Y_L_M, &out_l) < 0 ||
	    i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_Y_H_M, &out_h) < 0) {
		LOG_DBG("failed to read magn sample (Y axis)");
		return -EIO;
	}

	data->sample_magn_y = (int16_t)((uint16_t)(out_l) |
			      ((uint16_t)(out_h) << 8));

	if (i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_Z_L_M, &out_l) < 0 ||
	    i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_Z_H_M, &out_h) < 0) {
		LOG_DBG("failed to read magn sample (Z axis)");
		return -EIO;
	}

	data->sample_magn_z = (int16_t)((uint16_t)(out_l) |
			      ((uint16_t)(out_h) << 8));

#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
	data->sample_magn_fs = data->magn_fs;
#endif

	return 0;
}
#endif

#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
static inline int lsm9ds0_mfd_sample_fetch_temp(const struct device *dev)
{
	struct lsm9ds0_mfd_data *data = dev->data;
	const struct lsm9ds0_mfd_config *config = dev->config;
	uint8_t out_l, out_h;

	if (i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_TEMP_L_XM, &out_l) < 0 ||
	    i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_OUT_TEMP_H_XM, &out_h) < 0) {
		LOG_DBG("failed to read temperature sample\n");
		return -EIO;
	}

	data->sample_temp = (int16_t)((uint16_t)(out_l) |
			    ((uint16_t)(out_h) << 8));

	return 0;
}
#endif

static inline int lsm9ds0_mfd_sample_fetch_all(const struct device *dev)
{
#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
	if (lsm9ds0_mfd_sample_fetch_accel(dev) < 0) {
		return -EIO;
	}
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
	if (lsm9ds0_mfd_sample_fetch_magn(dev) < 0) {
		return -EIO;
	}
#endif

#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
	if (lsm9ds0_mfd_sample_fetch_temp(dev) < 0) {
		return -EIO;
	}
#endif

	return 0;
}

static int lsm9ds0_mfd_sample_fetch(const struct device *dev,
				    enum sensor_channel chan)
{
	switch (chan) {
#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm9ds0_mfd_sample_fetch_accel(dev);
#endif
#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
	case SENSOR_CHAN_MAGN_XYZ:
		return lsm9ds0_mfd_sample_fetch_magn(dev);
#endif
#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
	case SENSOR_CHAN_DIE_TEMP:
		return lsm9ds0_mfd_sample_fetch_temp(dev);
#endif
	case SENSOR_CHAN_ALL:
		return lsm9ds0_mfd_sample_fetch_all(dev);
	default:
		return -EINVAL;
	}

	return 0;
}

#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
static inline void lsm9ds0_mfd_convert_accel(struct sensor_value *val,
					     int raw_val,
					     float scale)
{
	double dval;

	dval = (double)(raw_val) * (double)scale;
	val->val1 = (int32_t)dval;
	val->val2 = ((int32_t)(dval * 1000000)) % 1000000;
}

static inline int lsm9ds0_mfd_get_accel_channel(enum sensor_channel chan,
						struct sensor_value *val,
						struct lsm9ds0_mfd_data *data,
						float scale)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lsm9ds0_mfd_convert_accel(val, data->sample_accel_x, scale);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lsm9ds0_mfd_convert_accel(val, data->sample_accel_y, scale);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lsm9ds0_mfd_convert_accel(val, data->sample_accel_z, scale);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm9ds0_mfd_convert_accel(val, data->sample_accel_x, scale);
		lsm9ds0_mfd_convert_accel(val + 1, data->sample_accel_y, scale);
		lsm9ds0_mfd_convert_accel(val + 2, data->sample_accel_z, scale);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline int lsm9ds0_mfd_get_accel(const struct device *dev,
					enum sensor_channel chan,
					struct sensor_value *val)
{
	struct lsm9ds0_mfd_data *data = dev->data;

#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
	switch (data->sample_accel_fs) {
	case 0:
		return lsm9ds0_mfd_get_accel_channel(chan, val, data,
						     2.0 * 9.807 / 32767.0);
	case 1:
		return lsm9ds0_mfd_get_accel_channel(chan, val, data,
						     4.0 * 9.807 / 32767.0);
	case 2:
		return lsm9ds0_mfd_get_accel_channel(chan, val, data,
						     6.0 * 9.807 / 32767.0);
	case 3:
		return lsm9ds0_mfd_get_accel_channel(chan, val, data,
						     8.0 * 9.807 / 32767.0);
	case 4:
		return lsm9ds0_mfd_get_accel_channel(chan, val, data,
						     16.0 * 9.807 / 32767.0);
	default:
		return -ENOTSUP;
	}
#elif defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_2)
	return lsm9ds0_mfd_get_accel_channel(chan, val, data,
					     2.0 * 9.807 / 32767.0);
#elif defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_4)
	return lsm9ds0_mfd_get_accel_channel(chan, val, data,
					     4.0 * 9.807 / 32767.0);
#elif defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_6)
	return lsm9ds0_mfd_get_accel_channel(chan, val, data,
					     6.0 * 9.807 / 32767.0);
#elif defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_8)
	return lsm9ds0_mfd_get_accel_channel(chan, val, data,
					     8.0 * 9.807 / 32767.0);
#elif defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_16)
	return lsm9ds0_mfd_get_accel_channel(chan, val, data,
					     16.0 * 9.807 / 32767.0);
#endif

	return 0;
}
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
static inline void lsm9ds0_mfd_convert_magn(struct sensor_value *val,
					    int raw_val,
					    float scale)
{
	double dval;

	dval = (double)(raw_val) * (double)scale;
	val->val1 = (int32_t)dval;
	val->val2 = ((int32_t)(dval * 1000000)) % 1000000;
}

static inline int lsm9ds0_mfd_get_magn_channel(enum sensor_channel chan,
					       struct sensor_value *val,
					       struct lsm9ds0_mfd_data *data,
					       float scale)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		lsm9ds0_mfd_convert_magn(val, data->sample_magn_x, scale);
		break;
	case SENSOR_CHAN_MAGN_Y:
		lsm9ds0_mfd_convert_magn(val, data->sample_magn_y, scale);
		break;
	case SENSOR_CHAN_MAGN_Z:
		lsm9ds0_mfd_convert_magn(val, data->sample_magn_z, scale);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		lsm9ds0_mfd_convert_magn(val, data->sample_magn_x, scale);
		lsm9ds0_mfd_convert_magn(val + 1, data->sample_magn_y, scale);
		lsm9ds0_mfd_convert_magn(val + 2, data->sample_magn_z, scale);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline int lsm9ds0_mfd_get_magn(const struct device *dev,
				       enum sensor_channel chan,
				       struct sensor_value *val)
{
	struct lsm9ds0_mfd_data *data = dev->data;

#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
	switch (data->sample_magn_fs) {
	case 0:
		return lsm9ds0_mfd_get_magn_channel(chan, val, data,
						    2.0 / 32767.0);
	case 1:
		return lsm9ds0_mfd_get_magn_channel(chan, val, data,
						    4.0 / 32767.0);
	case 2:
		return lsm9ds0_mfd_get_magn_channel(chan, val, data,
						    8.0 / 32767.0);
	case 3:
		return lsm9ds0_mfd_get_magn_channel(chan, val, data,
						    12.0 / 32767.0);
	default:
		return -ENOTSUP;
	}
#elif defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_2)
	return lsm9ds0_mfd_get_magn_channel(chan, val, data, 2.0 / 32767.0);
#elif defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_4)
	return lsm9ds0_mfd_get_magn_channel(chan, val, data, 4.0 / 32767.0);
#elif defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_8)
	return lsm9ds0_mfd_get_magn_channel(chan, val, data, 8.0 / 32767.0);
#elif defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_12)
	return lsm9ds0_mfd_get_magn_channel(chan, val, data, 12.0 / 32767.0);
#endif

	return 0;
}
#endif

static int lsm9ds0_mfd_channel_get(const struct device *dev,
				   enum sensor_channel chan,
				   struct sensor_value *val)
{
#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
	struct lsm9ds0_mfd_data *data = dev->data;
#endif

	switch (chan) {
#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm9ds0_mfd_get_accel(dev, chan, val);
#endif
#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		return lsm9ds0_mfd_get_magn(dev, chan, val);
#endif
#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
	case SENSOR_CHAN_DIE_TEMP:
		val->val1 = data->sample_temp;
		val->val2 = 0;
		return 0;
#endif
	default:
		return -ENOTSUP;
	}
}

#if defined(LSM9DS0_MFD_ATTR_SET_ACCEL)
static inline int lsm9ds0_mfd_attr_set_accel(const struct device *dev,
					     enum sensor_attribute attr,
					     const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_SAMPLING_RATE_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm9ds0_mfd_accel_set_odr(dev, val);
#endif
#if defined(CONFIG_LSM9DS0_MFD_ACCEL_FULL_SCALE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		return lsm9ds0_mfd_accel_set_fs(dev, sensor_ms2_to_g(val));
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#if defined(LSM9DS0_MFD_ATTR_SET_MAGN)
static inline int lsm9ds0_mfd_attr_set_magn(const struct device *dev,
					     enum sensor_attribute attr,
					     const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_LSM9DS0_MFD_MAGN_SAMPLING_RATE_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm9ds0_mfd_magn_set_odr(dev, val);
#endif
#if defined(CONFIG_LSM9DS0_MFD_MAGN_FULL_SCALE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		return lsm9ds0_mfd_magn_set_fs(dev, val);
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#if defined(LSM9DS0_MFD_ATTR_SET)
static int lsm9ds0_mfd_attr_set(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{

	switch (chan) {
#if defined(LSM9DS0_MFD_ATTR_SET_ACCEL)
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm9ds0_mfd_attr_set_accel(dev, attr, val);
#endif
#if defined(LSM9DS0_MFD_ATTR_SET_MAGN)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		return lsm9ds0_mfd_attr_set_magn(dev, attr, val);
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static const struct sensor_driver_api lsm9ds0_mfd_api_funcs = {
	.sample_fetch = lsm9ds0_mfd_sample_fetch,
	.channel_get = lsm9ds0_mfd_channel_get,
#if defined(LSM9DS0_MFD_ATTR_SET)
	.attr_set = lsm9ds0_mfd_attr_set,
#endif
};

static int lsm9ds0_mfd_init_chip(const struct device *dev)
{
	const struct lsm9ds0_mfd_config *config = dev->config;
	uint8_t chip_id;

	if (lsm9ds0_mfd_reboot_memory(dev) < 0) {
		LOG_DBG("failed to reset device");
		return -EIO;
	}

	if (i2c_reg_read_byte_dt(&config->i2c, LSM9DS0_MFD_REG_WHO_AM_I_XM, &chip_id) < 0) {
		LOG_DBG("failed reading chip id");
		return -EIO;
	}

	if (chip_id != LSM9DS0_MFD_VAL_WHO_AM_I_XM) {
		LOG_DBG("invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	LOG_DBG("chip id 0x%x", chip_id);

#if !defined(LSM9DS0_MFD_ACCEL_DISABLED)
	if (i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG1_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG1_XM_BDU |
				   LSM9DS0_MFD_MASK_CTRL_REG1_XM_AODR,
				   (1 << LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_BDU) |
				   (LSM9DS0_MFD_ACCEL_DEFAULT_AODR <<
				   LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AODR))) {
		LOG_DBG("failed to set AODR and BDU");
		return -EIO;
	}

	if (lsm9ds0_mfd_accel_set_fs_raw(dev, LSM9DS0_MFD_ACCEL_DEFAULT_FS)) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG1_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG1_XM_AXEN |
				   LSM9DS0_MFD_MASK_CTRL_REG1_XM_AYEN |
				   LSM9DS0_MFD_MASK_CTRL_REG1_XM_AZEN,
				   (LSM9DS0_MFD_ACCEL_ENABLE_X <<
				   LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AXEN) |
				   (LSM9DS0_MFD_ACCEL_ENABLE_Y <<
				   LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AYEN) |
				   (LSM9DS0_MFD_ACCEL_ENABLE_Z <<
				   LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_AZEN)) < 0) {
		LOG_DBG("failed to set accelerometer axis enable bits\n");
		return -EIO;
	}

#elif !defined(LSM9DS0_MFD_MAGN_DISABLED)
	if (i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG1_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG1_XM_BDU,
				   1 << LSM9DS0_MFD_SHIFT_CTRL_REG1_XM_BDU) < 0) {
		LOG_DBG("failed to set BDU\n");
		return -EIO;
	}
#endif

#if !defined(LSM9DS0_MFD_MAGN_DISABLED)
	if (i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG7_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG7_XM_MD,
				   (0 << LSM9DS0_MFD_SHIFT_CTRL_REG7_XM_MD)) < 0) {
		LOG_DBG("failed to power on magnetometer");
		return -EIO;
	}

	if (lsm9ds0_mfd_magn_set_odr_raw(dev, LSM9DS0_MFD_MAGN_DEFAULT_M_ODR)) {
		LOG_DBG("failed to set magnetometer sampling rate");
		return -EIO;
	}

	if (lsm9ds0_mfd_magn_set_fs_raw(dev, LSM9DS0_MFD_MAGN_DEFAULT_FS)) {
		LOG_DBG("failed to set magnetometer full-scale");
		return -EIO;
	}
#endif

#if !defined(LSM9DS0_MFD_TEMP_DISABLED)
	if (i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_MFD_REG_CTRL_REG5_XM,
				   LSM9DS0_MFD_MASK_CTRL_REG5_XM_TEMP_EN,
				   1 << LSM9DS0_MFD_SHIFT_CTRL_REG5_XM_TEMP_EN) < 0) {
		LOG_DBG("failed to power on temperature sensor");
		return -EIO;
	}
#endif

	return 0;
}

int lsm9ds0_mfd_init(const struct device *dev)
{
	const struct lsm9ds0_mfd_config * const config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (lsm9ds0_mfd_init_chip(dev) < 0) {
		LOG_DBG("failed to initialize chip");
		return -EIO;
	}

	return 0;
}

static const struct lsm9ds0_mfd_config lsm9ds0_mfd_config = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
};

static struct lsm9ds0_mfd_data lsm9ds0_mfd_data;

DEVICE_DT_INST_DEFINE(0, lsm9ds0_mfd_init, NULL,
		    &lsm9ds0_mfd_data, &lsm9ds0_mfd_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lsm9ds0_mfd_api_funcs);
