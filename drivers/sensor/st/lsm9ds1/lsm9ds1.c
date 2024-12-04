/*
 * Copyright (c) 2024 Bootlin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm9ds1

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "lsm9ds1.h"
#include <stmemsc.h>

LOG_MODULE_REGISTER(LSM9DS1, CONFIG_SENSOR_LOG_LEVEL);

/* Sensitivity of the accelerometer, indexed by the raw full scale value. Unit is µg/ LSB */
static const uint16_t lsm9ds1_accel_fs_sens[] = {61, 732, 122, 244};

/*
 * Sensitivity of the gyroscope, indexed by the raw full scale value.
 * The value here is just a factor applied to GAIN_UNIT_G, as the sensitivity is
 * proportional to the full scale size.
 * The index 2 is never used : the value `0` is just a placeholder.
 */
static const uint16_t lsm9ds1_gyro_fs_sens[] = {1, 2, 0, 8};

/*
 * Values of the different sampling frequencies of the accelerometer, indexed by the raw odr
 * value that the sensor understands.
 */
static const uint16_t lsm9ds1_odr_map[] = {0, 10, 50, 119, 238, 476, 952};

/*
 * Value of the different sampling frequencies of the gyroscope, indexed by the raw odr value
 * that the sensor understands.
 */
static const uint16_t lsm9ds1_gyro_odr_map[] = {0, 15, 59, 119, 238, 476, 952};

static int lsm9ds1_reboot(const struct device *dev)
{
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm9ds1_ctrl_reg8_t ctrl8_reg;
	int ret;

	ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG8, (uint8_t *)&ctrl8_reg, 1);
	if (ret < 0) {
		return ret;
	}

	ctrl8_reg.boot = 1;

	ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG8, (uint8_t *)&ctrl8_reg, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(50);

	return 0;
}

static int lsm9ds1_accel_range_to_fs_val(int32_t range)
{
	switch (range) {
	case 2:
		return LSM9DS1_2g;
	case 4:
		return LSM9DS1_4g;
	case 8:
		return LSM9DS1_8g;
	case 16:
		return LSM9DS1_16g;
	default:
		return -EINVAL;
	}
}

static int lsm9ds1_gyro_range_to_fs_val(int32_t range)
{
	switch (range) {
	case 245:
		return LSM9DS1_245dps;
	case 500:
		return LSM9DS1_500dps;
	case 2000:
		return LSM9DS1_2000dps;
	default:
		return -EINVAL;
	}
}

static int lsm9ds1_accel_fs_val_to_gain(int fs)
{
	return lsm9ds1_accel_fs_sens[fs];
}

static int lsm9ds1_accel_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds1_odr_map); i++) {
		if (freq <= lsm9ds1_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lsm9ds1_gyro_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm9ds1_gyro_odr_map); i++) {
		if (freq <= lsm9ds1_gyro_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lsm9ds1_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_data *data = dev->data;
	int ret;

	lsm9ds1_ctrl_reg6_xl_t ctrl_reg6_xl;

	ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG6_XL, (uint8_t *)&ctrl_reg6_xl, 1);
	if (ret < 0) {
		return ret;
	}

	ctrl_reg6_xl.odr_xl = odr;

	ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG6_XL, (uint8_t *)&ctrl_reg6_xl, 1);
	if (ret < 0) {
		return ret;
	}

	data->accel_odr = odr;

	return 0;
}

static int lsm9ds1_gyro_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_data *data = dev->data;
	int ret;

	lsm9ds1_ctrl_reg1_g_t ctrl_reg1;

	ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_G, (uint8_t *)&ctrl_reg1, 1);
	if (ret < 0) {
		return ret;
	}

	ctrl_reg1.odr_g = odr;

	ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG1_G, (uint8_t *)&ctrl_reg1, 1);
	if (ret < 0) {
		return ret;
	}

	data->gyro_odr = odr;
	return 0;
}

static int lsm9ds1_gyro_odr_set(const struct device *dev, uint16_t freq)
{
	struct lsm9ds1_data *data = dev->data;
	int odr;
	int ret;

	odr = lsm9ds1_gyro_freq_to_odr_val(freq);

	if (odr == data->gyro_odr) {
		return 0;
	}

	LOG_INF("You are also changing the odr of the accelerometer");

	ret = lsm9ds1_gyro_set_odr_raw(dev, odr);
	if (ret < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return ret;
	}

	/*
	 * When the gyroscope is on, the value of the accelerometer odr must be
	 * the same as the value of the gyroscope.
	 */

	ret = lsm9ds1_accel_set_odr_raw(dev, odr);
	if (ret < 0) {
		LOG_ERR("failed to set accelerometer sampling rate");
		return ret;
	}

	return 0;
}

static int lsm9ds1_accel_odr_set(const struct device *dev, uint16_t freq)
{
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_data *data = dev->data;
	int odr, ret;
	lsm9ds1_imu_odr_t old_odr;

	ret = lsm9ds1_imu_data_rate_get(ctx, &old_odr);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The gyroscope is on :
	 * we have to change the odr on both the accelerometer and the gyroscope
	 */
	if (old_odr & GYRO_ODR_MASK) {

		odr = lsm9ds1_gyro_freq_to_odr_val(freq);

		if (odr == data->gyro_odr) {
			return 0;
		}

		LOG_INF("You are also changing the odr of the gyroscope");

		ret = lsm9ds1_accel_set_odr_raw(dev, odr);
		if (ret < 0) {
			LOG_DBG("failed to set accelerometer sampling rate");
			return ret;
		}

		ret = lsm9ds1_gyro_set_odr_raw(dev, odr);
		if (ret < 0) {
			LOG_ERR("failed to set gyroscope sampling rate");
			return ret;
		}

	/* The gyroscope is off, we have to change the odr of just the accelerometer */
	} else {

		odr = lsm9ds1_accel_freq_to_odr_val(freq);

		if (odr == data->accel_odr) {
			return 0;
		}

		if (odr < 0) {
			return odr;
		}

		ret = lsm9ds1_accel_set_odr_raw(dev, odr);
		if (ret < 0) {
			LOG_DBG("failed to set accelerometer sampling rate");
			return ret;
		}
	}
	return 0;
}

static int lsm9ds1_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lsm9ds1_data *data = dev->data;
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	fs = lsm9ds1_accel_range_to_fs_val(range);
	if (fs < 0) {
		LOG_DBG("full scale value not supported");
		return fs;
	}

	ret = lsm9ds1_xl_full_scale_set(ctx, fs);
	if (ret < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return ret;
	}

	data->acc_gain = lsm9ds1_accel_fs_val_to_gain(fs);
	return 0;
}

static int lsm9ds1_gyro_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lsm9ds1_data *data = dev->data;
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	fs = lsm9ds1_gyro_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	ret = lsm9ds1_gy_full_scale_set(ctx, fs);
	if (ret < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return ret;
	}

	data->gyro_gain = (lsm9ds1_gyro_fs_sens[fs] * GAIN_UNIT_G);
	return 0;
}

static int lsm9ds1_accel_config(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lsm9ds1_accel_range_set(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm9ds1_accel_odr_set(dev, val->val1);
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm9ds1_gyro_config(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lsm9ds1_gyro_range_set(dev, sensor_rad_to_degrees(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm9ds1_gyro_odr_set(dev, val->val1);
	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm9ds1_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm9ds1_accel_config(dev, chan, attr, val);
	case SENSOR_CHAN_GYRO_XYZ:
		return lsm9ds1_gyro_config(dev, chan, attr, val);
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}
	return 0;
}

static int lsm9ds1_sample_fetch_accel(const struct device *dev)
{
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_data *data = dev->data;
	int ret;

	ret = lsm9ds1_acceleration_raw_get(ctx, data->acc);
	if (ret < 0) {
		LOG_DBG("Failed to read sample");
		return ret;
	}

	return 0;
}

static int lsm9ds1_sample_fetch_gyro(const struct device *dev)
{
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_data *data = dev->data;
	int ret;

	ret = lsm9ds1_angular_rate_raw_get(ctx, data->gyro);
	if (ret < 0) {
		LOG_DBG("Failed to read sample");
		return ret;
	}
	return 0;
}

#if IS_ENABLED(CONFIG_LSM9DS1_ENABLE_TEMP)
static int lsm9ds1_sample_fetch_temp(const struct device *dev)
{
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_data *data = dev->data;
	int ret;

	ret = lsm9ds1_temperature_raw_get(ctx, &data->temp_sample);
	if (ret < 0) {
		LOG_DBG("Failed to read sample");
		return ret;
	}

	return 0;
}
#endif

static int lsm9ds1_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm9ds1_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm9ds1_sample_fetch_gyro(dev);
		break;
#if IS_ENABLED(CONFIG_LSM9DS1_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm9ds1_sample_fetch_temp(dev);
		break;
#endif
	case SENSOR_CHAN_ALL:
		lsm9ds1_sample_fetch_accel(dev);
		lsm9ds1_sample_fetch_gyro(dev);
#if IS_ENABLED(CONFIG_LSM9DS1_ENABLE_TEMP)
		lsm9ds1_sample_fetch_temp(dev);
#endif
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static inline void lsm9ds1_accel_convert(struct sensor_value *val, int raw_val,
					 uint32_t sensitivity)
{
	/* Sensitivity is exposed in ug/LSB */
	/* Convert to m/s^2 */
	sensor_ug_to_ms2(raw_val * sensitivity, val);
}

static inline int lsm9ds1_accel_get_channel(enum sensor_channel chan, struct sensor_value *val,
					    struct lsm9ds1_data *data, uint32_t sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lsm9ds1_accel_convert(val, data->acc[0], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lsm9ds1_accel_convert(val, data->acc[1], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lsm9ds1_accel_convert(val, data->acc[2], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		for (i = 0; i < 3; i++) {
			lsm9ds1_accel_convert(val++, data->acc[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm9ds1_accel_channel_get(enum sensor_channel chan, struct sensor_value *val,
				     struct lsm9ds1_data *data)
{
	return lsm9ds1_accel_get_channel(chan, val, data, data->acc_gain);
}

static inline void lsm9ds1_gyro_convert(struct sensor_value *val, int raw_val, uint32_t sensitivity)
{
	/* Sensitivity is exposed in udps/LSB */
	/* Convert to rad/s */
	sensor_10udegrees_to_rad((raw_val * (int32_t)sensitivity) / 10, val);
}

static inline int lsm9ds1_gyro_get_channel(enum sensor_channel chan, struct sensor_value *val,
					   struct lsm9ds1_data *data, uint32_t sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		lsm9ds1_gyro_convert(val, data->gyro[0], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Y:
		lsm9ds1_gyro_convert(val, data->gyro[1], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Z:
		lsm9ds1_gyro_convert(val, data->gyro[2], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		for (i = 0; i < 3; i++) {
			lsm9ds1_gyro_convert(val++, data->gyro[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm9ds1_gyro_channel_get(enum sensor_channel chan, struct sensor_value *val,
				    struct lsm9ds1_data *data)
{
	return lsm9ds1_gyro_get_channel(chan, val, data, data->gyro_gain);
}

#if IS_ENABLED(CONFIG_LSM9DS1_ENABLE_TEMP)
static void lsm9ds1_temp_channel_get(struct sensor_value *val, struct lsm9ds1_data *data)
{
	val->val1 = data->temp_sample / TEMP_SENSITIVITY + TEMP_OFFSET;
	val->val2 = (data->temp_sample % TEMP_SENSITIVITY) * (1000000 / TEMP_SENSITIVITY);
}
#endif

static int lsm9ds1_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lsm9ds1_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm9ds1_accel_channel_get(chan, val, data);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		lsm9ds1_gyro_channel_get(chan, val, data);
		break;
#if IS_ENABLED(CONFIG_LSM9DS1_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm9ds1_temp_channel_get(val, data);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, lsm9ds1_api_funcs) = {
	.sample_fetch = lsm9ds1_sample_fetch,
	.channel_get = lsm9ds1_channel_get,
	.attr_set = lsm9ds1_attr_set,
};

static int lsm9ds1_init(const struct device *dev)
{
	const struct lsm9ds1_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_data *lsm9ds1 = dev->data;
	uint8_t chip_id, fs;
	int ret;

	ret = lsm9ds1_reboot(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reboot device");
		return ret;
	}

	ret = lsm9ds1_read_reg(ctx, LSM9DS1_WHO_AM_I, &chip_id, 1);
	if (ret < 0) {
		LOG_ERR("failed reading chip id");
		return ret;
	}

	if (chip_id != LSM9DS1_IMU_ID) {
		LOG_ERR("Invalid ID : got %x", chip_id);
		return -EIO;
	}
	LOG_DBG("chip_id : %x", chip_id);

	LOG_DBG("output data rate is %d\n", cfg->imu_odr);
	ret = lsm9ds1_imu_data_rate_set(ctx, cfg->imu_odr);
	if (ret < 0) {
		LOG_ERR("failed to set IMU odr");
		return ret;
	}

	fs = cfg->accel_range;
	LOG_DBG("accel range is %d\n", fs);
	ret = lsm9ds1_xl_full_scale_set(ctx, fs);
	if (ret < 0) {
		LOG_ERR("failed to set accelerometer range %d", fs);
		return ret;
	}

	lsm9ds1->acc_gain = lsm9ds1_accel_fs_val_to_gain(fs);

	fs = cfg->gyro_range;
	LOG_DBG("gyro range is %d", fs);
	ret = lsm9ds1_gy_full_scale_set(ctx, fs);
	if (ret < 0) {
		LOG_ERR("failed to set gyroscope range %d\n", fs);
		return ret;
	}
	lsm9ds1->gyro_gain = (lsm9ds1_gyro_fs_sens[fs] * GAIN_UNIT_G);

	return 0;
}

#define LSM9DS1_CONFIG_COMMON(inst)                                                                \
	.imu_odr = DT_INST_PROP(inst, imu_odr),							   \
	.accel_range = DT_INST_PROP(inst, accel_range),						   \
	.gyro_range = DT_INST_PROP(inst, gyro_range),

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LSM9DS1_CONFIG_I2C(inst)                                                                   \
	{                                                                                          \
		STMEMSC_CTX_I2C(&lsm9ds1_config_##inst.stmemsc_cfg),                               \
		.stmemsc_cfg =                                                                     \
			{                                                                          \
				.i2c = I2C_DT_SPEC_INST_GET(inst),                                 \
			},                                                                         \
		LSM9DS1_CONFIG_COMMON(inst)                                                        \
	}

#define LSM9DS1_DEFINE(inst)                                                                       \
	static struct lsm9ds1_data lsm9ds1_data_##inst = {                                         \
		.acc_gain = 0,                                                                     \
	};                                                                                         \
                                                                                                   \
	static struct lsm9ds1_config lsm9ds1_config_##inst = LSM9DS1_CONFIG_I2C(inst);             \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lsm9ds1_init, NULL, &lsm9ds1_data_##inst,               \
				     &lsm9ds1_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &lsm9ds1_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(LSM9DS1_DEFINE);
