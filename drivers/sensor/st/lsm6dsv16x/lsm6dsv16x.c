/* ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv16x.pdf
 */

#define DT_DRV_COMPAT st_lsm6dsv16x

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "lsm6dsv16x.h"

LOG_MODULE_REGISTER(LSM6DSV16X, CONFIG_SENSOR_LOG_LEVEL);

/*
 * values taken from lsm6dsv16x_data_rate_t in hal/st module. The mode/accuracy
 * should be selected through accel-odr property in DT
 */
static const float lsm6dsv16x_odr_map[3][13] = {
			/* High Accuracy off */
			{0.0f, 1.875f, 7.5f, 15.0f, 30.0f, 60.0f,
			 120.0f, 240.0f, 480.0f, 960.0f, 1920.0f,
			 3840.0f, 7680.0f},

			/* High Accuracy 1 */
			{0.0f, 1.875f, 7.5f, 15.625f, 31.25f, 62.5f,
			 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f,
			 4000.0f, 8000.0f},

			/* High Accuracy 2 */
			{0.0f, 1.875f, 7.5f, 12.5f, 25.0f, 50.0f,
			 100.0f, 200.0f, 400.0f, 800.0f, 1600.0f,
			 3200.0f, 6400.0f},
		};

static int lsm6dsv16x_freq_to_odr_val(const struct device *dev, uint16_t freq)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsv16x_data_rate_t odr;
	int8_t mode;
	size_t i;

	if (lsm6dsv16x_xl_data_rate_get(ctx, &odr) < 0) {
		return -EINVAL;
	}

	mode = (odr >> 4) & 0xf;

	for (i = 0; i < ARRAY_SIZE(lsm6dsv16x_odr_map[mode]); i++) {
		if (freq <= lsm6dsv16x_odr_map[mode][i]) {
			LOG_DBG("mode: %d - odr: %d", mode, i);
			return i;
		}
	}

	return -EINVAL;
}

static const uint16_t lsm6dsv16x_accel_fs_map[] = {2, 4, 8, 16};

static int lsm6dsv16x_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dsv16x_accel_fs_map); i++) {
		if (range == lsm6dsv16x_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static const uint16_t lsm6dsv16x_gyro_fs_map[] = {125, 250, 500, 1000, 2000, 0,   0,
						  0,   0,   0,   0,    0,    4000};
static const uint16_t lsm6dsv16x_gyro_fs_sens[] = {1, 2, 4, 8, 16, 0, 0, 0, 0, 0, 0, 0, 32};

static int lsm6dsv16x_gyro_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dsv16x_gyro_fs_map); i++) {
		if (range == lsm6dsv16x_gyro_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lsm6dsv16x_accel_set_fs_raw(const struct device *dev, uint8_t fs)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsv16x_data *data = dev->data;
	lsm6dsv16x_xl_full_scale_t val;

	switch (fs) {
	case 0:
		val = LSM6DSV16X_2g;
		break;
	case 1:
		val = LSM6DSV16X_4g;
		break;
	case 2:
		val = LSM6DSV16X_8g;
		break;
	case 3:
		val = LSM6DSV16X_16g;
		break;
	default:
		return -EIO;
	}

	if (lsm6dsv16x_xl_full_scale_set(ctx, val) < 0) {
		return -EIO;
	}

	data->accel_fs = fs;

	return 0;
}

static int lsm6dsv16x_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsv16x_data *data = dev->data;

	if (lsm6dsv16x_xl_data_rate_set(ctx, odr) < 0) {
		return -EIO;
	}

	data->accel_freq = odr;

	return 0;
}

static int lsm6dsv16x_gyro_set_fs_raw(const struct device *dev, uint8_t fs)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsv16x_data *data = dev->data;

	if (lsm6dsv16x_gy_full_scale_set(ctx, fs) < 0) {
		return -EIO;
	}

	data->gyro_fs = fs;
	return 0;
}

static int lsm6dsv16x_gyro_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (lsm6dsv16x_gy_data_rate_set(ctx, odr) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsv16x_accel_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = lsm6dsv16x_freq_to_odr_val(dev, freq);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dsv16x_accel_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}

static int lsm6dsv16x_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lsm6dsv16x_data *data = dev->data;

	fs = lsm6dsv16x_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lsm6dsv16x_accel_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	data->acc_gain = lsm6dsv16x_accel_fs_map[fs] * GAIN_UNIT_XL / 2;
	return 0;
}

static int lsm6dsv16x_accel_config(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsv16x_xl_mode_t mode;

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dsv16x_accel_range_set(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dsv16x_accel_odr_set(dev, val->val1);
	case SENSOR_ATTR_CONFIGURATION:
		switch (val->val1) {
		case 0: /* High Performance */
			mode = LSM6DSV16X_XL_HIGH_PERFORMANCE_MD;
			break;
		case 1: /* High Accuracy */
			mode = LSM6DSV16X_XL_HIGH_ACCURACY_ODR_MD;
			break;
		case 3: /* ODR triggered */
			mode = LSM6DSV16X_XL_ODR_TRIGGERED_MD;
			break;
		case 4: /* Low Power 2 */
			mode = LSM6DSV16X_XL_LOW_POWER_2_AVG_MD;
			break;
		case 5: /* Low Power 4 */
			mode = LSM6DSV16X_XL_LOW_POWER_4_AVG_MD;
			break;
		case 6: /* Low Power 8 */
			mode = LSM6DSV16X_XL_LOW_POWER_8_AVG_MD;
			break;
		case 7: /* Normal */
			mode = LSM6DSV16X_XL_NORMAL_MD;
			break;
		default:
			return -EIO;
		}

		return lsm6dsv16x_xl_mode_set(ctx, mode);
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsv16x_gyro_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	if (freq < 8) {
		return -EIO;
	}

	odr = lsm6dsv16x_freq_to_odr_val(dev, freq);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dsv16x_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	return 0;
}

static int lsm6dsv16x_gyro_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lsm6dsv16x_data *data = dev->data;

	fs = lsm6dsv16x_gyro_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lsm6dsv16x_gyro_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	data->gyro_gain = (lsm6dsv16x_gyro_fs_sens[fs] * GAIN_UNIT_G);
	return 0;
}

static int lsm6dsv16x_gyro_config(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsv16x_gy_mode_t mode;

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dsv16x_gyro_range_set(dev, sensor_rad_to_degrees(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dsv16x_gyro_odr_set(dev, val->val1);
	case SENSOR_ATTR_CONFIGURATION:
		switch (val->val1) {
		case 0: /* High Performance */
			mode = LSM6DSV16X_GY_HIGH_PERFORMANCE_MD;
			break;
		case 1: /* High Accuracy */
			mode = LSM6DSV16X_GY_HIGH_ACCURACY_ODR_MD;
			break;
		case 4: /* Sleep */
			mode = LSM6DSV16X_GY_SLEEP_MD;
			break;
		case 5: /* Low Power */
			mode = LSM6DSV16X_GY_LOW_POWER_MD;
			break;
		default:
			return -EIO;
		}

		return lsm6dsv16x_gy_mode_set(ctx, mode);
	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsv16x_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
#if defined(CONFIG_LSM6DSV16X_SENSORHUB)
	struct lsm6dsv16x_data *data = dev->data;
#endif /* CONFIG_LSM6DSV16X_SENSORHUB */

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm6dsv16x_accel_config(dev, chan, attr, val);
	case SENSOR_CHAN_GYRO_XYZ:
		return lsm6dsv16x_gyro_config(dev, chan, attr, val);
#if defined(CONFIG_LSM6DSV16X_SENSORHUB)
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_PRESS:
	case SENSOR_CHAN_HUMIDITY:
		if (!data->shub_inited) {
			LOG_ERR("shub not inited.");
			return -ENOTSUP;
		}

		return lsm6dsv16x_shub_config(dev, chan, attr, val);
#endif /* CONFIG_LSM6DSV16X_SENSORHUB */
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsv16x_sample_fetch_accel(const struct device *dev)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsv16x_data *data = dev->data;

	if (lsm6dsv16x_acceleration_raw_get(ctx, data->acc) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	return 0;
}

static int lsm6dsv16x_sample_fetch_gyro(const struct device *dev)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsv16x_data *data = dev->data;

	if (lsm6dsv16x_angular_rate_raw_get(ctx, data->gyro) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	return 0;
}

#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
static int lsm6dsv16x_sample_fetch_temp(const struct device *dev)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsv16x_data *data = dev->data;

	if (lsm6dsv16x_temperature_raw_get(ctx, &data->temp_sample) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	return 0;
}
#endif

#if defined(CONFIG_LSM6DSV16X_SENSORHUB)
static int lsm6dsv16x_sample_fetch_shub(const struct device *dev)
{
	if (lsm6dsv16x_shub_fetch_external_devs(dev) < 0) {
		LOG_DBG("failed to read ext shub devices");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_LSM6DSV16X_SENSORHUB */

static int lsm6dsv16x_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
#if defined(CONFIG_LSM6DSV16X_SENSORHUB)
	struct lsm6dsv16x_data *data = dev->data;
#endif /* CONFIG_LSM6DSV16X_SENSORHUB */

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsv16x_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsv16x_sample_fetch_gyro(dev);
		break;
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm6dsv16x_sample_fetch_temp(dev);
		break;
#endif
	case SENSOR_CHAN_ALL:
		lsm6dsv16x_sample_fetch_accel(dev);
		lsm6dsv16x_sample_fetch_gyro(dev);
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
		lsm6dsv16x_sample_fetch_temp(dev);
#endif
#if defined(CONFIG_LSM6DSV16X_SENSORHUB)
		if (data->shub_inited) {
			lsm6dsv16x_sample_fetch_shub(dev);
		}
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lsm6dsv16x_accel_convert(struct sensor_value *val, int raw_val,
					 uint32_t sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in ug/LSB */
	/* Convert to m/s^2 */
	dval = (int64_t)(raw_val) * sensitivity;
	sensor_ug_to_ms2(dval, val);
}

static inline int lsm6dsv16x_accel_get_channel(enum sensor_channel chan,
					    struct sensor_value *val,
					    struct lsm6dsv16x_data *data,
					    uint32_t sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lsm6dsv16x_accel_convert(val, data->acc[0], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lsm6dsv16x_accel_convert(val, data->acc[1], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lsm6dsv16x_accel_convert(val, data->acc[2], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		for (i = 0; i < 3; i++) {
			lsm6dsv16x_accel_convert(val++, data->acc[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsv16x_accel_channel_get(enum sensor_channel chan,
				     struct sensor_value *val,
				     struct lsm6dsv16x_data *data)
{
	return lsm6dsv16x_accel_get_channel(chan, val, data, data->acc_gain);
}

static inline void lsm6dsv16x_gyro_convert(struct sensor_value *val, int raw_val,
					uint32_t sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in udps/LSB */
	/* So, calculate value in 10 udps unit and then to rad/s */
	dval = (int64_t)(raw_val) * sensitivity / 10;
	sensor_10udegrees_to_rad(dval, val);
}

static inline int lsm6dsv16x_gyro_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm6dsv16x_data *data,
					   uint32_t sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		lsm6dsv16x_gyro_convert(val, data->gyro[0], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Y:
		lsm6dsv16x_gyro_convert(val, data->gyro[1], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Z:
		lsm6dsv16x_gyro_convert(val, data->gyro[2], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		for (i = 0; i < 3; i++) {
			lsm6dsv16x_gyro_convert(val++, data->gyro[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsv16x_gyro_channel_get(enum sensor_channel chan,
				    struct sensor_value *val,
				    struct lsm6dsv16x_data *data)
{
	return lsm6dsv16x_gyro_get_channel(chan, val, data, data->gyro_gain);
}

#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
static void lsm6dsv16x_gyro_channel_get_temp(struct sensor_value *val,
					  struct lsm6dsv16x_data *data)
{
	int32_t micro_c;

	/* convert units to micro Celsius. Raw temperature samples are
	 * expressed in 256 LSB/deg_C units. And LSB output is 0 at 25 C.
	 */
	micro_c = (data->temp_sample * 1000000) / 256;

	val->val1 = micro_c / 1000000 + 25;
	val->val2 = micro_c % 1000000;
}
#endif

#if defined(CONFIG_LSM6DSV16X_SENSORHUB)
static inline void lsm6dsv16x_magn_convert(struct sensor_value *val, int raw_val,
					uint16_t sensitivity)
{
	double dval;

	/* Sensitivity is exposed in ugauss/LSB */
	dval = (double)(raw_val * sensitivity);
	val->val1 = (int32_t)dval / 1000000;
	val->val2 = (int32_t)dval % 1000000;
}

static inline int lsm6dsv16x_magn_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm6dsv16x_data *data)
{
	int16_t sample[3];
	int idx;

	idx = lsm6dsv16x_shub_get_idx(data->dev, SENSOR_CHAN_MAGN_XYZ);
	if (idx < 0) {
		LOG_DBG("external magn not supported");
		return -ENOTSUP;
	}


	sample[0] = (int16_t)(data->ext_data[idx][0] |
			     (data->ext_data[idx][1] << 8));
	sample[1] = (int16_t)(data->ext_data[idx][2] |
			     (data->ext_data[idx][3] << 8));
	sample[2] = (int16_t)(data->ext_data[idx][4] |
			     (data->ext_data[idx][5] << 8));

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		lsm6dsv16x_magn_convert(val, sample[0], data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_Y:
		lsm6dsv16x_magn_convert(val, sample[1], data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_Z:
		lsm6dsv16x_magn_convert(val, sample[2], data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		lsm6dsv16x_magn_convert(val, sample[0], data->magn_gain);
		lsm6dsv16x_magn_convert(val + 1, sample[1], data->magn_gain);
		lsm6dsv16x_magn_convert(val + 2, sample[2], data->magn_gain);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lsm6dsv16x_hum_convert(struct sensor_value *val,
				       struct lsm6dsv16x_data *data)
{
	float rh;
	int16_t raw_val;
	struct hts221_data *ht = &data->hts221;
	int idx;

	idx = lsm6dsv16x_shub_get_idx(data->dev, SENSOR_CHAN_HUMIDITY);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = (int16_t)(data->ext_data[idx][0] |
			   (data->ext_data[idx][1] << 8));

	/* find relative humidty by linear interpolation */
	rh = (ht->y1 - ht->y0) * raw_val + ht->x1 * ht->y0 - ht->x0 * ht->y1;
	rh /= (ht->x1 - ht->x0);

	/* convert humidity to integer and fractional part */
	val->val1 = rh;
	val->val2 = rh * 1000000;
}

static inline void lsm6dsv16x_press_convert(struct sensor_value *val,
					 struct lsm6dsv16x_data *data)
{
	int32_t raw_val;
	int idx;

	idx = lsm6dsv16x_shub_get_idx(data->dev, SENSOR_CHAN_PRESS);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = (int32_t)(data->ext_data[idx][0] |
			   (data->ext_data[idx][1] << 8) |
			   (data->ext_data[idx][2] << 16));

	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Convert raw_val to val in kPa */
	val->val1 = (raw_val >> 12) / 10;
	val->val2 = (raw_val >> 12) % 10 * 100000 +
		(((int32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void lsm6dsv16x_temp_convert(struct sensor_value *val,
					struct lsm6dsv16x_data *data)
{
	int16_t raw_val;
	int idx;

	idx = lsm6dsv16x_shub_get_idx(data->dev, SENSOR_CHAN_PRESS);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = (int16_t)(data->ext_data[idx][3] |
			   (data->ext_data[idx][4] << 8));

	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = (int32_t)raw_val % 100 * (10000);
}
#endif

static int lsm6dsv16x_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lsm6dsv16x_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsv16x_accel_channel_get(chan, val, data);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsv16x_gyro_channel_get(chan, val, data);
		break;
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm6dsv16x_gyro_channel_get_temp(val, data);
		break;
#endif
#if defined(CONFIG_LSM6DSV16X_SENSORHUB)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		if (!data->shub_inited) {
			LOG_ERR("attr_set() shub not inited.");
			return -ENOTSUP;
		}

		lsm6dsv16x_magn_get_channel(chan, val, data);
		break;

	case SENSOR_CHAN_HUMIDITY:
		if (!data->shub_inited) {
			LOG_ERR("attr_set() shub not inited.");
			return -ENOTSUP;
		}

		lsm6dsv16x_hum_convert(val, data);
		break;

	case SENSOR_CHAN_PRESS:
		if (!data->shub_inited) {
			LOG_ERR("attr_set() shub not inited.");
			return -ENOTSUP;
		}

		lsm6dsv16x_press_convert(val, data);
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		if (!data->shub_inited) {
			LOG_ERR("attr_set() shub not inited.");
			return -ENOTSUP;
		}

		lsm6dsv16x_temp_convert(val, data);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lsm6dsv16x_driver_api = {
	.attr_set = lsm6dsv16x_attr_set,
#if CONFIG_LSM6DSV16X_TRIGGER
	.trigger_set = lsm6dsv16x_trigger_set,
#endif
	.sample_fetch = lsm6dsv16x_sample_fetch,
	.channel_get = lsm6dsv16x_channel_get,
};

static int lsm6dsv16x_init_chip(const struct device *dev)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	uint8_t chip_id;
	uint8_t odr, fs;

	/* All registers except 0x01 are different between banks, including the WHO_AM_I
	 * register and the register used for a SW reset.  If the lsm6dsv16x wasn't on the user
	 * bank when it reset, then both the chip id check and the sw reset will fail unless we
	 * set the bank now.
	 */
	if (lsm6dsv16x_mem_bank_set(ctx, LSM6DSV16X_MAIN_MEM_BANK) < 0) {
		LOG_DBG("Failed to set user bank");
		return -EIO;
	}

	if (lsm6dsv16x_device_id_get(ctx, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		return -EIO;
	}

	LOG_INF("chip id 0x%x", chip_id);

	if (chip_id != LSM6DSV16X_ID) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/* reset device (sw_por) */
	if (lsm6dsv16x_reset_set(ctx, LSM6DSV16X_GLOBAL_RST) < 0) {
		return -EIO;
	}

	/* wait 30ms as reported in AN5763 */
	k_sleep(K_MSEC(30));

	fs = cfg->accel_range;
	LOG_DBG("accel range is %d", fs);
	if (lsm6dsv16x_accel_set_fs_raw(dev, fs) < 0) {
		LOG_ERR("failed to set accelerometer range %d", fs);
		return -EIO;
	}
	lsm6dsv16x->acc_gain = lsm6dsv16x_accel_fs_map[fs] * GAIN_UNIT_XL / 2;

	odr = cfg->accel_odr;
	LOG_DBG("accel odr is %d", odr);
	if (lsm6dsv16x_accel_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set accelerometer odr %d", odr);
		return -EIO;
	}

	fs = cfg->gyro_range;
	LOG_DBG("gyro range is %d", fs);
	if (lsm6dsv16x_gyro_set_fs_raw(dev, fs) < 0) {
		LOG_ERR("failed to set gyroscope range %d", fs);
		return -EIO;
	}
	lsm6dsv16x->gyro_gain = (lsm6dsv16x_gyro_fs_sens[fs] * GAIN_UNIT_G);

	odr = cfg->gyro_odr;
	LOG_DBG("gyro odr is %d", odr);
	lsm6dsv16x->gyro_freq = odr;
	if (lsm6dsv16x_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set gyroscope odr %d", odr);
		return -EIO;
	}

	if (lsm6dsv16x_block_data_update_set(ctx, 1) < 0) {
		LOG_DBG("failed to set BDU mode");
		return -EIO;
	}

	return 0;
}

static int lsm6dsv16x_init(const struct device *dev)
{
#ifdef CONFIG_LSM6DSV16X_TRIGGER
	const struct lsm6dsv16x_config *cfg = dev->config;
#endif
	struct lsm6dsv16x_data *data = dev->data;

	LOG_INF("Initialize device %s", dev->name);
	data->dev = dev;

	if (lsm6dsv16x_init_chip(dev) < 0) {
		LOG_DBG("failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LSM6DSV16X_TRIGGER
	if (cfg->trig_enabled) {
		if (lsm6dsv16x_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return -EIO;
		}
	}
#endif

#ifdef CONFIG_LSM6DSV16X_SENSORHUB
	data->shub_inited = true;
	if (lsm6dsv16x_shub_init(dev) < 0) {
		LOG_INF("shub: no external chips found");
		data->shub_inited = false;
	}
#endif

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LSM6DSV16X driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by LSM6DSV16X_DEFINE_SPI() and
 * LSM6DSV16X_DEFINE_I2C().
 */

#define LSM6DSV16X_DEVICE_INIT(inst)					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			    lsm6dsv16x_init,				\
			    NULL,					\
			    &lsm6dsv16x_data_##inst,			\
			    &lsm6dsv16x_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &lsm6dsv16x_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_LSM6DSV16X_TRIGGER
#define LSM6DSV16X_CFG_IRQ(inst)					\
	.trig_enabled = true,						\
	.int1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, { 0 }),	\
	.int2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, { 0 }),	\
	.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed),                 \
	.drdy_pin = DT_INST_PROP(inst, drdy_pin)
#else
#define LSM6DSV16X_CFG_IRQ(inst)
#endif /* CONFIG_LSM6DSV16X_TRIGGER */

#define LSM6DSV16X_SPI_OP  (SPI_WORD_SET(8) |				\
			 SPI_OP_MODE_MASTER |				\
			 SPI_MODE_CPOL |				\
			 SPI_MODE_CPHA)					\

#define LSM6DSV16X_CONFIG_COMMON(inst)					\
	.accel_odr = DT_INST_PROP(inst, accel_odr),			\
	.accel_range = DT_INST_PROP(inst, accel_range),			\
	.gyro_odr = DT_INST_PROP(inst, gyro_odr),			\
	.gyro_range = DT_INST_PROP(inst, gyro_range),			\
	IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
			   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),	\
		   (LSM6DSV16X_CFG_IRQ(inst)))

#define LSM6DSV16X_CONFIG_SPI(inst)					\
	{								\
		STMEMSC_CTX_SPI(&lsm6dsv16x_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.spi = SPI_DT_SPEC_INST_GET(inst,		\
					   LSM6DSV16X_SPI_OP,		\
					   0),				\
		},							\
		LSM6DSV16X_CONFIG_COMMON(inst)				\
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LSM6DSV16X_CONFIG_I2C(inst)					\
	{								\
		STMEMSC_CTX_I2C(&lsm6dsv16x_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		},							\
		LSM6DSV16X_CONFIG_COMMON(inst)				\
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LSM6DSV16X_DEFINE(inst)						\
	static struct lsm6dsv16x_data lsm6dsv16x_data_##inst;			\
	static const struct lsm6dsv16x_config lsm6dsv16x_config_##inst =	\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
			(LSM6DSV16X_CONFIG_SPI(inst)),			\
			(LSM6DSV16X_CONFIG_I2C(inst)));			\
	LSM6DSV16X_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(LSM6DSV16X_DEFINE)
