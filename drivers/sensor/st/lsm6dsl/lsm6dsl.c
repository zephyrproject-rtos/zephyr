/* lsm6dsl.c - Driver for LSM6DSL accelerometer, gyroscope and
 * temperature sensor
 */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm6dsl

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include "lsm6dsl.h"

LOG_MODULE_REGISTER(LSM6DSL, CONFIG_SENSOR_LOG_LEVEL);

static const uint16_t lsm6dsl_odr_map[] = {0, 12, 26, 52, 104, 208, 416, 833,
					1666, 3332, 6664, 1};

#if defined(LSM6DSL_ACCEL_ODR_RUNTIME) || defined(LSM6DSL_GYRO_ODR_RUNTIME) ||\
	defined(CONFIG_PM_DEVICE)
static int lsm6dsl_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dsl_odr_map); i++) {
		if (freq == lsm6dsl_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static int lsm6dsl_odr_to_freq_val(uint16_t odr)
{
	/* for valid index, return value from map */
	if (odr < ARRAY_SIZE(lsm6dsl_odr_map)) {
		return lsm6dsl_odr_map[odr];
	}

	/* invalid index, return the fastest entry (6.66kHz) */
	BUILD_ASSERT(ARRAY_SIZE(lsm6dsl_odr_map) > 10);
	return lsm6dsl_odr_map[10];
}

#ifdef LSM6DSL_ACCEL_FS_RUNTIME
static const uint16_t lsm6dsl_accel_fs_map[] = {2, 16, 4, 8};
static const uint16_t lsm6dsl_accel_fs_sens[] = {1, 8, 2, 4};

static int lsm6dsl_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dsl_accel_fs_map); i++) {
		if (range == lsm6dsl_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

#ifdef LSM6DSL_GYRO_FS_RUNTIME
static const uint16_t lsm6dsl_gyro_fs_map[] = {250, 500, 1000, 2000, 125};
static const uint16_t lsm6dsl_gyro_fs_sens[] = {2, 4, 8, 16, 1};

static int lsm6dsl_gyro_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dsl_gyro_fs_map); i++) {
		if (range == lsm6dsl_gyro_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static inline int lsm6dsl_reboot(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;

	if (data->hw_tf->update_reg(dev, LSM6DSL_REG_CTRL3_C,
				    LSM6DSL_MASK_CTRL3_C_BOOT,
				    1 << LSM6DSL_SHIFT_CTRL3_C_BOOT) < 0) {
		return -EIO;
	}

	/* Wait sensor turn-on time as per datasheet */
	k_busy_wait(USEC_PER_MSEC * 35U);

	return 0;
}

static int lsm6dsl_accel_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct lsm6dsl_data *data = dev->data;

	if (data->hw_tf->update_reg(dev,
				    LSM6DSL_REG_CTRL1_XL,
				    LSM6DSL_MASK_CTRL1_XL_FS_XL,
				    fs << LSM6DSL_SHIFT_CTRL1_XL_FS_XL) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct lsm6dsl_data *data = dev->data;

	if (data->hw_tf->update_reg(dev,
				    LSM6DSL_REG_CTRL1_XL,
				    LSM6DSL_MASK_CTRL1_XL_ODR_XL,
				    odr << LSM6DSL_SHIFT_CTRL1_XL_ODR_XL) < 0) {
		return -EIO;
	}

	data->accel_freq = lsm6dsl_odr_to_freq_val(odr);

	return 0;
}

static int lsm6dsl_gyro_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct lsm6dsl_data *data = dev->data;

	if (fs == GYRO_FULLSCALE_125) {
		if (data->hw_tf->update_reg(dev,
					LSM6DSL_REG_CTRL2_G,
					LSM6DSL_MASK_CTRL2_FS125 | LSM6DSL_MASK_CTRL2_G_FS_G,
					1 << LSM6DSL_SHIFT_CTRL2_FS125) < 0) {
			return -EIO;
		}
	} else {
		if (data->hw_tf->update_reg(dev,
					LSM6DSL_REG_CTRL2_G,
					LSM6DSL_MASK_CTRL2_FS125 | LSM6DSL_MASK_CTRL2_G_FS_G,
					fs << LSM6DSL_SHIFT_CTRL2_G_FS_G) < 0) {
			return -EIO;
		}
	}

	return 0;
}

static int lsm6dsl_gyro_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct lsm6dsl_data *data = dev->data;

	if (data->hw_tf->update_reg(dev,
				    LSM6DSL_REG_CTRL2_G,
				    LSM6DSL_MASK_CTRL2_G_ODR_G,
				    odr << LSM6DSL_SHIFT_CTRL2_G_ODR_G) < 0) {
		return -EIO;
	}

	data->gyro_freq = lsm6dsl_odr_to_freq_val(odr);

	return 0;
}

#ifdef LSM6DSL_ACCEL_ODR_RUNTIME
static int lsm6dsl_accel_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = lsm6dsl_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dsl_accel_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef LSM6DSL_ACCEL_FS_RUNTIME
static int lsm6dsl_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lsm6dsl_data *data = dev->data;

	fs = lsm6dsl_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lsm6dsl_accel_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	data->accel_sensitivity = (float)(lsm6dsl_accel_fs_sens[fs]
						    * SENSI_GRAIN_XL);
	return 0;
}
#endif

static int lsm6dsl_accel_config(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	switch (attr) {
#ifdef LSM6DSL_ACCEL_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dsl_accel_range_set(dev, sensor_ms2_to_g(val));
#endif
#ifdef LSM6DSL_ACCEL_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dsl_accel_odr_set(dev, val->val1);
#endif
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

#ifdef LSM6DSL_GYRO_ODR_RUNTIME
static int lsm6dsl_gyro_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = lsm6dsl_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dsl_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef LSM6DSL_GYRO_FS_RUNTIME
static int lsm6dsl_gyro_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lsm6dsl_data *data = dev->data;

	fs = lsm6dsl_gyro_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lsm6dsl_gyro_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	data->gyro_sensitivity = (float)(lsm6dsl_gyro_fs_sens[fs]
						    * SENSI_GRAIN_G);
	return 0;
}
#endif

static int lsm6dsl_gyro_config(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	switch (attr) {
#ifdef LSM6DSL_GYRO_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dsl_gyro_range_set(dev, sensor_rad_to_degrees(val));
#endif
#ifdef LSM6DSL_GYRO_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dsl_gyro_odr_set(dev, val->val1);
#endif
	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsl_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm6dsl_accel_config(dev, chan, attr, val);
	case SENSOR_CHAN_GYRO_XYZ:
		return lsm6dsl_gyro_config(dev, chan, attr, val);
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsl_sample_fetch_accel(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t buf[6];

	if (data->hw_tf->read_data(dev, LSM6DSL_REG_OUTX_L_XL,
				   buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->accel_sample_x = (int16_t)((uint16_t)(buf[0]) |
				((uint16_t)(buf[1]) << 8));
	data->accel_sample_y = (int16_t)((uint16_t)(buf[2]) |
				((uint16_t)(buf[3]) << 8));
	data->accel_sample_z = (int16_t)((uint16_t)(buf[4]) |
				((uint16_t)(buf[5]) << 8));

	return 0;
}

static int lsm6dsl_sample_fetch_gyro(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t buf[6];

	if (data->hw_tf->read_data(dev, LSM6DSL_REG_OUTX_L_G,
				   buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->gyro_sample_x = (int16_t)((uint16_t)(buf[0]) |
				((uint16_t)(buf[1]) << 8));
	data->gyro_sample_y = (int16_t)((uint16_t)(buf[2]) |
				((uint16_t)(buf[3]) << 8));
	data->gyro_sample_z = (int16_t)((uint16_t)(buf[4]) |
				((uint16_t)(buf[5]) << 8));

	return 0;
}

#if defined(CONFIG_LSM6DSL_ENABLE_TEMP)
static int lsm6dsl_sample_fetch_temp(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t buf[2];

	if (data->hw_tf->read_data(dev, LSM6DSL_REG_OUT_TEMP_L,
				   buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->temp_sample = (int16_t)((uint16_t)(buf[0]) |
				((uint16_t)(buf[1]) << 8));

	return 0;
}
#endif

#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL) || defined(CONFIG_LSM6DSL_EXT0_LIS3MDL)
static int lsm6dsl_sample_fetch_magn(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t buf[6];

	if (lsm6dsl_shub_read_external_chip(dev, buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read ext mag sample");
		return -EIO;
	}

	data->magn_sample_x = (int16_t)((uint16_t)(buf[0]) |
				((uint16_t)(buf[1]) << 8));
	data->magn_sample_y = (int16_t)((uint16_t)(buf[2]) |
				((uint16_t)(buf[3]) << 8));
	data->magn_sample_z = (int16_t)((uint16_t)(buf[4]) |
				((uint16_t)(buf[5]) << 8));

	return 0;
}
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
static int lsm6dsl_sample_fetch_press(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t buf[5];

	if (lsm6dsl_shub_read_external_chip(dev, buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read ext press sample");
		return -EIO;
	}

	data->sample_press = (int32_t)((uint32_t)(buf[0]) |
				     ((uint32_t)(buf[1]) << 8) |
				     ((uint32_t)(buf[2]) << 16));
	data->sample_temp = (int16_t)((uint16_t)(buf[3]) |
				     ((uint16_t)(buf[4]) << 8));

	return 0;
}
#endif

static int lsm6dsl_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsl_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsl_sample_fetch_gyro(dev);
		break;
#if defined(CONFIG_LSM6DSL_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm6dsl_sample_fetch_temp(dev);
		break;
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL) || defined(CONFIG_LSM6DSL_EXT0_LIS3MDL)
	case SENSOR_CHAN_MAGN_XYZ:
		lsm6dsl_sample_fetch_magn(dev);
		break;
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_PRESS:
		lsm6dsl_sample_fetch_press(dev);
		break;
#endif
	case SENSOR_CHAN_ALL:
		lsm6dsl_sample_fetch_accel(dev);
		lsm6dsl_sample_fetch_gyro(dev);
#if defined(CONFIG_LSM6DSL_ENABLE_TEMP)
		lsm6dsl_sample_fetch_temp(dev);
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL) || defined(CONFIG_LSM6DSL_EXT0_LIS3MDL)
		lsm6dsl_sample_fetch_magn(dev);
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
		lsm6dsl_sample_fetch_press(dev);
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lsm6dsl_accel_convert(struct sensor_value *val, int raw_val,
					 float sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in ug/LSB */
	/* Convert to m/s^2 */
	dval = (int64_t)raw_val * sensitivity;
	sensor_ug_to_ms2(dval, val);
}

static inline int lsm6dsl_accel_get_channel(enum sensor_channel chan,
					    struct sensor_value *val,
					    struct lsm6dsl_data *data,
					    float sensitivity)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lsm6dsl_accel_convert(val, data->accel_sample_x, sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lsm6dsl_accel_convert(val, data->accel_sample_y, sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lsm6dsl_accel_convert(val, data->accel_sample_z, sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsl_accel_convert(val, data->accel_sample_x, sensitivity);
		lsm6dsl_accel_convert(val + 1, data->accel_sample_y,
				      sensitivity);
		lsm6dsl_accel_convert(val + 2, data->accel_sample_z,
				      sensitivity);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsl_accel_channel_get(enum sensor_channel chan,
				     struct sensor_value *val,
				     struct lsm6dsl_data *data)
{
	return lsm6dsl_accel_get_channel(chan, val, data,
					data->accel_sensitivity);
}

static inline void lsm6dsl_gyro_convert(struct sensor_value *val, int raw_val,
					float sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in udps/LSB */
	/* So, calculate value in 10 udps unit and then to rad/s */
	dval = (int64_t)raw_val * sensitivity / 10;
	sensor_10udegrees_to_rad(dval, val);
}

static inline int lsm6dsl_gyro_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm6dsl_data *data,
					   float sensitivity)
{
	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		lsm6dsl_gyro_convert(val, data->gyro_sample_x, sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Y:
		lsm6dsl_gyro_convert(val, data->gyro_sample_y, sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Z:
		lsm6dsl_gyro_convert(val, data->gyro_sample_z, sensitivity);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsl_gyro_convert(val, data->gyro_sample_x, sensitivity);
		lsm6dsl_gyro_convert(val + 1, data->gyro_sample_y, sensitivity);
		lsm6dsl_gyro_convert(val + 2, data->gyro_sample_z, sensitivity);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsl_gyro_channel_get(enum sensor_channel chan,
				    struct sensor_value *val,
				    struct lsm6dsl_data *data)
{
	return lsm6dsl_gyro_get_channel(chan, val, data,
					data->gyro_sensitivity);
}

#if defined(CONFIG_LSM6DSL_ENABLE_TEMP)
static void lsm6dsl_gyro_channel_get_temp(struct sensor_value *val,
					  struct lsm6dsl_data *data)
{
	/* val = temp_sample / 256 + 25 */
	val->val1 = data->temp_sample / 256 + 25;
	val->val2 = (data->temp_sample % 256) * (1000000 / 256);
}
#endif

#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL) || defined(CONFIG_LSM6DSL_EXT0_LIS3MDL)
static inline void lsm6dsl_magn_convert(struct sensor_value *val, int raw_val,
					float sensitivity)
{
	double dval;

	/* Sensitivity is exposed in mgauss/LSB */
	dval = (double)(raw_val * sensitivity);
	val->val1 = (int32_t)dval / 1000000;
	val->val2 = (int32_t)dval % 1000000;
}

static inline int lsm6dsl_magn_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm6dsl_data *data)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		lsm6dsl_magn_convert(val,
				     data->magn_sample_x,
				     data->magn_sensitivity);
		break;
	case SENSOR_CHAN_MAGN_Y:
		lsm6dsl_magn_convert(val,
				     data->magn_sample_y,
				     data->magn_sensitivity);
		break;
	case SENSOR_CHAN_MAGN_Z:
		lsm6dsl_magn_convert(val,
				     data->magn_sample_z,
				     data->magn_sensitivity);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		lsm6dsl_magn_convert(val,
				     data->magn_sample_x,
				     data->magn_sensitivity);
		lsm6dsl_magn_convert(val + 1,
				     data->magn_sample_y,
				     data->magn_sensitivity);
		lsm6dsl_magn_convert(val + 2,
				     data->magn_sample_z,
				     data->magn_sensitivity);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsl_magn_channel_get(enum sensor_channel chan,
				    struct sensor_value *val,
				    struct lsm6dsl_data *data)
{
	return lsm6dsl_magn_get_channel(chan, val, data);
}
#endif

#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
static inline void lps22hb_press_convert(struct sensor_value *val,
					 int32_t raw_val)
{
	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Convert raw_val to val in kPa */
	val->val1 = (raw_val >> 12) / 10;
	val->val2 = (raw_val >> 12) % 10 * 100000 +
		(((int32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void lps22hb_temp_convert(struct sensor_value *val,
					int16_t raw_val)
{
	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = (int32_t)raw_val % 100 * (10000);
}
#endif

static int lsm6dsl_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lsm6dsl_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsl_accel_channel_get(chan, val, data);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsl_gyro_channel_get(chan, val, data);
		break;
#if defined(CONFIG_LSM6DSL_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm6dsl_gyro_channel_get_temp(val, data);
		break;
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL) || defined(CONFIG_LSM6DSL_EXT0_LIS3MDL)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		lsm6dsl_magn_channel_get(chan, val, data);
		break;
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
	case SENSOR_CHAN_PRESS:
		lps22hb_press_convert(val, data->sample_press);
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		lps22hb_temp_convert(val, data->sample_temp);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lsm6dsl_driver_api = {
	.attr_set = lsm6dsl_attr_set,
#if CONFIG_LSM6DSL_TRIGGER
	.trigger_set = lsm6dsl_trigger_set,
#endif
	.sample_fetch = lsm6dsl_sample_fetch,
	.channel_get = lsm6dsl_channel_get,
};

static int lsm6dsl_init_chip(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;
	uint8_t chip_id;

	if (lsm6dsl_reboot(dev) < 0) {
		LOG_DBG("failed to reboot device");
		return -EIO;
	}

	if (data->hw_tf->read_reg(dev, LSM6DSL_REG_WHO_AM_I, &chip_id) < 0) {
		LOG_DBG("failed reading chip id");
		return -EIO;
	}
	if (chip_id != LSM6DSL_VAL_WHO_AM_I) {
		LOG_DBG("invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	LOG_DBG("chip id 0x%x", chip_id);

	if (lsm6dsl_accel_set_fs_raw(dev,
				     LSM6DSL_DEFAULT_ACCEL_FULLSCALE) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}
	data->accel_sensitivity = LSM6DSL_DEFAULT_ACCEL_SENSITIVITY;

	if (lsm6dsl_accel_set_odr_raw(dev, CONFIG_LSM6DSL_ACCEL_ODR) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	if (lsm6dsl_gyro_set_fs_raw(dev, LSM6DSL_DEFAULT_GYRO_FULLSCALE) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}
	data->gyro_sensitivity = LSM6DSL_DEFAULT_GYRO_SENSITIVITY;

	if (lsm6dsl_gyro_set_odr_raw(dev, CONFIG_LSM6DSL_GYRO_ODR) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	if (data->hw_tf->update_reg(dev,
				LSM6DSL_REG_FIFO_CTRL5,
				LSM6DSL_MASK_FIFO_CTRL5_FIFO_MODE,
				0 << LSM6DSL_SHIFT_FIFO_CTRL5_FIFO_MODE) < 0) {
		LOG_DBG("failed to set FIFO mode");
		return -EIO;
	}

	if (data->hw_tf->update_reg(dev,
				    LSM6DSL_REG_CTRL3_C,
				    LSM6DSL_MASK_CTRL3_C_BDU |
				    LSM6DSL_MASK_CTRL3_C_BLE |
				    LSM6DSL_MASK_CTRL3_C_IF_INC,
				    (1 << LSM6DSL_SHIFT_CTRL3_C_BDU) |
				    (0 << LSM6DSL_SHIFT_CTRL3_C_BLE) |
				    (1 << LSM6DSL_SHIFT_CTRL3_C_IF_INC)) < 0) {
		LOG_DBG("failed to set BDU, BLE and burst");
		return -EIO;
	}

	if (data->hw_tf->update_reg(dev,
				    LSM6DSL_REG_CTRL6_C,
				    LSM6DSL_MASK_CTRL6_C_XL_HM_MODE,
				    (1 << LSM6DSL_SHIFT_CTRL6_C_XL_HM_MODE)) < 0) {
		LOG_DBG("failed to disable accelerometer high performance mode");
		return -EIO;
	}

	if (data->hw_tf->update_reg(dev,
				    LSM6DSL_REG_CTRL7_G,
				    LSM6DSL_MASK_CTRL7_G_HM_MODE,
				    (1 << LSM6DSL_SHIFT_CTRL7_G_HM_MODE)) < 0) {
		LOG_DBG("failed to disable gyroscope high performance mode");
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_init(const struct device *dev)
{
	int ret;
	const struct lsm6dsl_config * const config = dev->config;

	ret = config->bus_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize sensor bus");
		return ret;
	}

	ret = lsm6dsl_init_chip(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize chip");
		return ret;
	}

#ifdef CONFIG_LSM6DSL_TRIGGER
	ret = lsm6dsl_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return ret;
	}
#endif

#ifdef CONFIG_LSM6DSL_SENSORHUB
	ret = lsm6dsl_shub_init_external_chip(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize external chip");
		return ret;
	}
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int lsm6dsl_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	struct lsm6dsl_data *data = dev->data;
	int ret = -EIO;
	uint8_t accel_odr = 0;
	uint8_t gyro_odr = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Restore saved ODR values */
		accel_odr = lsm6dsl_freq_to_odr_val(data->accel_freq);
		ret = lsm6dsl_accel_set_odr_raw(dev, accel_odr);
		if (ret < 0) {
			LOG_ERR("Failed to resume accelerometer");
			break;
		}
		gyro_odr = lsm6dsl_freq_to_odr_val(data->gyro_freq);
		ret = lsm6dsl_gyro_set_odr_raw(dev, gyro_odr);
		if (ret < 0) {
			LOG_ERR("Failed to resume gyro");
			break;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/*
		 * Set accelerometer ODR to power-down. Don't use the direct
		 * function to not overwrite the saved value
		 */
		ret = data->hw_tf->update_reg(dev, LSM6DSL_REG_CTRL1_XL,
					      LSM6DSL_MASK_CTRL1_XL_ODR_XL, 0);
		if (ret < 0) {
			LOG_ERR("Failed to suspend accelerometer");
			break;
		}
		/* Set gyro ODR to power-down */
		ret = data->hw_tf->update_reg(dev, LSM6DSL_REG_CTRL2_G,
					      LSM6DSL_MASK_CTRL2_G_ODR_G, 0);
		if (ret < 0) {
			LOG_ERR("Failed to suspend gyro");
			break;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif


#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LSM6DSL driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by LSM6DSL_DEFINE_SPI() and
 * LSM6DSL_DEFINE_I2C().
 */

#define LSM6DSL_DEVICE_INIT(inst)					\
	PM_DEVICE_DT_INST_DEFINE(inst, lsm6dsl_pm_action);		\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			    lsm6dsl_init,				\
			    PM_DEVICE_DT_INST_GET(inst),		\
			    &lsm6dsl_data_##inst,			\
			    &lsm6dsl_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &lsm6dsl_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_LSM6DSL_TRIGGER
#define LSM6DSL_CFG_IRQ(inst) \
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),
#else
#define LSM6DSL_CFG_IRQ(inst)
#endif /* CONFIG_LSM6DSL_TRIGGER */

#define LSM6DSL_CONFIG_SPI(inst)						\
	{									\
		.bus_init = lsm6dsl_spi_init,					\
		.bus_cfg.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) |	\
				      SPI_OP_MODE_MASTER |			\
				      SPI_MODE_CPOL |				\
				      SPI_MODE_CPHA, 0),			\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),		\
		(LSM6DSL_CFG_IRQ(inst)), ())					\
	}

#define LSM6DSL_DEFINE_SPI(inst)					\
	static struct lsm6dsl_data lsm6dsl_data_##inst;			\
	static const struct lsm6dsl_config lsm6dsl_config_##inst =	\
		LSM6DSL_CONFIG_SPI(inst);				\
	LSM6DSL_DEVICE_INIT(inst)

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LSM6DSL_CONFIG_I2C(inst)					\
	{								\
		.bus_init = lsm6dsl_i2c_init,				\
		.bus_cfg.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),	\
		(LSM6DSL_CFG_IRQ(inst)), ())				\
	}

#define LSM6DSL_DEFINE_I2C(inst)					\
	static struct lsm6dsl_data lsm6dsl_data_##inst;			\
	static const struct lsm6dsl_config lsm6dsl_config_##inst =	\
		LSM6DSL_CONFIG_I2C(inst);				\
	LSM6DSL_DEVICE_INIT(inst)
/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LSM6DSL_DEFINE(inst)						\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (LSM6DSL_DEFINE_SPI(inst)),				\
		    (LSM6DSL_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(LSM6DSL_DEFINE)
