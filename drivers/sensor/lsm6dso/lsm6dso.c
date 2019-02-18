/* ST Microelectronics LSM6DSO 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <logging/log.h>

#include "lsm6dso.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(LSM6DSO);

static const u16_t lsm6dso_odr_map[] = {0, 12, 26, 52, 104, 208, 416, 833,
					1660, 3330, 6660};

#if defined(LSM6DSO_ACCEL_ODR_RUNTIME) || defined(LSM6DSO_GYRO_ODR_RUNTIME)
static int lsm6dso_freq_to_odr_val(u16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dso_odr_map); i++) {
		if (freq == lsm6dso_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static int lsm6dso_odr_to_freq_val(u16_t odr)
{
	/* for valid index, return value from map */
	if (odr < ARRAY_SIZE(lsm6dso_odr_map)) {
		return lsm6dso_odr_map[odr];
	}

	/* invalid index, return last entry */
	return lsm6dso_odr_map[ARRAY_SIZE(lsm6dso_odr_map) - 1];
}

#ifdef LSM6DSO_ACCEL_FS_RUNTIME
static const u16_t lsm6dso_accel_fs_map[] = {2, 16, 4, 8};
static const u16_t lsm6dso_accel_fs_sens[] = {1, 8, 2, 4};

static int lsm6dso_accel_range_to_fs_val(s32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dso_accel_fs_map); i++) {
		if (range == lsm6dso_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

#ifdef LSM6DSO_GYRO_FS_RUNTIME
static const u16_t lsm6dso_gyro_fs_map[] = {250, 500, 1000, 2000, 125};
static const u16_t lsm6dso_gyro_fs_sens[] = {2, 4, 8, 16, 1};

static int lsm6dso_gyro_range_to_fs_val(s32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dso_gyro_fs_map); i++) {
		if (range == lsm6dso_gyro_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static inline int lsm6dso_reboot(struct device *dev)
{
	struct lsm6dso_data *data = dev->driver_data;

	if (data->hw_tf->update_reg(data, LSM6DSO_REG_CTRL3_C,
				    LSM6DSO_MASK_CTRL3_C_BOOT, 1) < 0) {
		return -EIO;
	}

	/* Wait sensor turn-on time as per datasheet */
	k_busy_wait(35 * USEC_PER_MSEC);

	return 0;
}

static int lsm6dso_accel_set_fs_raw(struct device *dev, u8_t fs)
{
	struct lsm6dso_data *data = dev->driver_data;

	if (data->hw_tf->update_reg(data,
				    LSM6DSO_REG_CTRL1_XL,
				    LSM6DSO_MASK_CTRL1_XL_FS_XL, fs) < 0) {
		return -EIO;
	}

	data->accel_fs = fs;

	return 0;
}

static int lsm6dso_accel_set_odr_raw(struct device *dev, u8_t odr)
{
	struct lsm6dso_data *data = dev->driver_data;

	if (data->hw_tf->update_reg(data, LSM6DSO_REG_CTRL1_XL,
				    LSM6DSO_MASK_CTRL1_XL_ODR_XL, odr) < 0) {
		return -EIO;
	}

	data->accel_freq = lsm6dso_odr_to_freq_val(odr);

	return 0;
}

static int lsm6dso_gyro_set_fs_raw(struct device *dev, u8_t fs)
{
	struct lsm6dso_data *data = dev->driver_data;

	if (fs == GYRO_FULLSCALE_125) {
		if (data->hw_tf->update_reg(data,
					LSM6DSO_REG_CTRL2_G,
					LSM6DSO_MASK_CTRL2_FS125, 1) < 0) {
			return -EIO;
		}
	} else {
		if (data->hw_tf->update_reg(data,
					LSM6DSO_REG_CTRL2_G,
					LSM6DSO_MASK_CTRL2_FS125, 0) < 0) {
			return -EIO;
		}
		if (data->hw_tf->update_reg(data,
					LSM6DSO_REG_CTRL2_G,
					LSM6DSO_MASK_CTRL2_G_FS_G, fs) < 0) {
			return -EIO;
		}
	}

	return 0;
}

static int lsm6dso_gyro_set_odr_raw(struct device *dev, u8_t odr)
{
	struct lsm6dso_data *data = dev->driver_data;

	if (data->hw_tf->update_reg(data,
				    LSM6DSO_REG_CTRL2_G,
				    LSM6DSO_MASK_CTRL2_G_ODR_G, odr) < 0) {
		return -EIO;
	}

	return 0;
}

#ifdef LSM6DSO_ACCEL_ODR_RUNTIME
static int lsm6dso_accel_odr_set(struct device *dev, u16_t freq)
{
	int odr;

	odr = lsm6dso_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dso_accel_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef LSM6DSO_ACCEL_FS_RUNTIME
static int lsm6dso_accel_range_set(struct device *dev, s32_t range)
{
	int fs;
	struct lsm6dso_data *data = dev->driver_data;

	fs = lsm6dso_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lsm6dso_accel_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	data->acc_gain = (float)(lsm6dso_accel_fs_sens[fs] * GAIN_UNIT_XL);
	return 0;
}
#endif

static int lsm6dso_accel_config(struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
#ifdef LSM6DSO_ACCEL_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dso_accel_range_set(dev, sensor_ms2_to_g(val));
#endif
#ifdef LSM6DSO_ACCEL_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dso_accel_odr_set(dev, val->val1);
#endif
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

#ifdef LSM6DSO_GYRO_ODR_RUNTIME
static int lsm6dso_gyro_odr_set(struct device *dev, u16_t freq)
{
	int odr;

	odr = lsm6dso_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dso_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef LSM6DSO_GYRO_FS_RUNTIME
static int lsm6dso_gyro_range_set(struct device *dev, s32_t range)
{
	int fs;
	struct lsm6dso_data *data = dev->driver_data;

	fs = lsm6dso_gyro_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lsm6dso_gyro_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	data->gyro_gain = (float)(lsm6dso_gyro_fs_sens[fs] * GAIN_UNIT_G);
	return 0;
}
#endif

static int lsm6dso_gyro_config(struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
#ifdef LSM6DSO_GYRO_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dso_gyro_range_set(dev, sensor_rad_to_degrees(val));
#endif
#ifdef LSM6DSO_GYRO_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dso_gyro_odr_set(dev, val->val1);
#endif
	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dso_attr_set(struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm6dso_accel_config(dev, chan, attr, val);
	case SENSOR_CHAN_GYRO_XYZ:
		return lsm6dso_gyro_config(dev, chan, attr, val);
#if defined(CONFIG_LSM6DSO_SENSORHUB)
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_PRESS:
	case SENSOR_CHAN_HUMIDITY:
		return lsm6dso_shub_config(dev, chan, attr, val);
#endif /* CONFIG_LSM6DSO_SENSORHUB */
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dso_sample_fetch_accel(struct device *dev)
{
	struct lsm6dso_data *data = dev->driver_data;
	union samples buf;

	if (data->hw_tf->read_data(data, LSM6DSO_REG_OUTX_L_XL,
				   buf.raw, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->acc[0] = sys_le16_to_cpu(buf.axis[0]);
	data->acc[1] = sys_le16_to_cpu(buf.axis[1]);
	data->acc[2] = sys_le16_to_cpu(buf.axis[2]);

	return 0;
}

static int lsm6dso_sample_fetch_gyro(struct device *dev)
{
	struct lsm6dso_data *data = dev->driver_data;
	union samples buf;

	if (data->hw_tf->read_data(data, LSM6DSO_REG_OUTX_L_G,
				   buf.raw, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->gyro[0] = sys_le16_to_cpu(buf.axis[0]);
	data->gyro[1] = sys_le16_to_cpu(buf.axis[1]);
	data->gyro[2] = sys_le16_to_cpu(buf.axis[2]);

	return 0;
}

#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
static int lsm6dso_sample_fetch_temp(struct device *dev)
{
	struct lsm6dso_data *data = dev->driver_data;
	u8_t buf[2];

	if (data->hw_tf->read_data(data, LSM6DSO_REG_OUT_TEMP_L,
				   buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->temp_sample = (s16_t)((u16_t)(buf[0]) |
				((u16_t)(buf[1]) << 8));

	return 0;
}
#endif

#if defined(CONFIG_LSM6DSO_SENSORHUB)
static int lsm6dso_sample_fetch_shub(struct device *dev)
{
	if (lsm6dso_shub_fetch_external_devs(dev) < 0) {
		LOG_DBG("failed to read ext shub devices");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_LSM6DSO_SENSORHUB */

static int lsm6dso_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dso_sample_fetch_accel(dev);
#if defined(CONFIG_LSM6DSO_SENSORHUB)
		lsm6dso_sample_fetch_shub(dev);
#endif
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dso_sample_fetch_gyro(dev);
		break;
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm6dso_sample_fetch_temp(dev);
		break;
#endif
	case SENSOR_CHAN_ALL:
		lsm6dso_sample_fetch_accel(dev);
		lsm6dso_sample_fetch_gyro(dev);
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
		lsm6dso_sample_fetch_temp(dev);
#endif
#if defined(CONFIG_LSM6DSO_SENSORHUB)
		lsm6dso_sample_fetch_shub(dev);
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lsm6dso_accel_convert(struct sensor_value *val, int raw_val,
					 float sensitivity)
{
	double dval;

	/* Sensitivity is exposed in mg/LSB */
	/* Convert to m/s^2 */
	dval = (double)(raw_val) * sensitivity * SENSOR_G_DOUBLE / 1000;
	val->val1 = (s32_t)dval;
	val->val2 = (((s32_t)(dval * 1000)) % 1000) * 1000;

}

static inline int lsm6dso_accel_get_channel(enum sensor_channel chan,
					    struct sensor_value *val,
					    struct lsm6dso_data *data,
					    float sensitivity)
{
	u8_t i;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lsm6dso_accel_convert(val, data->acc[0], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lsm6dso_accel_convert(val, data->acc[1], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lsm6dso_accel_convert(val, data->acc[2], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		for (i = 0; i < 3; i++) {
			lsm6dso_accel_convert(val++, data->acc[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dso_accel_channel_get(enum sensor_channel chan,
				     struct sensor_value *val,
				     struct lsm6dso_data *data)
{
	return lsm6dso_accel_get_channel(chan, val, data,
					data->acc_gain);
}

static inline void lsm6dso_gyro_convert(struct sensor_value *val, int raw_val,
					float sensitivity)
{
	double dval;

	/* Sensitivity is exposed in mdps/LSB */
	/* Convert to rad/s */
	dval = (double)(raw_val * sensitivity * SENSOR_DEG2RAD_DOUBLE / 1000);
	val->val1 = (s32_t)dval;
	val->val2 = (((s32_t)(dval * 1000)) % 1000) * 1000;
}

static inline int lsm6dso_gyro_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm6dso_data *data,
					   float sensitivity)
{
	u8_t i;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		lsm6dso_gyro_convert(val, data->gyro[0], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Y:
		lsm6dso_gyro_convert(val, data->gyro[1], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Z:
		lsm6dso_gyro_convert(val, data->gyro[2], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		for (i = 0; i < 3; i++) {
			lsm6dso_gyro_convert(val++, data->gyro[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dso_gyro_channel_get(enum sensor_channel chan,
				    struct sensor_value *val,
				    struct lsm6dso_data *data)
{
	return lsm6dso_gyro_get_channel(chan, val, data,
					LSM6DSO_DEFAULT_GYRO_SENSITIVITY);
}

#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
static void lsm6dso_gyro_channel_get_temp(struct sensor_value *val,
					  struct lsm6dso_data *data)
{
	/* val = temp_sample / 256 + 25 */
	val->val1 = data->temp_sample / 256 + 25;
	val->val2 = (data->temp_sample % 256) * (1000000 / 256);
}
#endif

#if defined(CONFIG_LSM6DSO_SENSORHUB)
static inline void lsm6dso_magn_convert(struct sensor_value *val, int raw_val,
					float sensitivity)
{
	double dval;

	/* Sensitivity is exposed in mgauss/LSB */
	dval = (double)(raw_val * sensitivity);
	val->val1 = (s32_t)dval / 1000000;
	val->val2 = (s32_t)dval % 1000000;
}

static inline int lsm6dso_magn_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm6dso_data *data)
{
	s16_t sample[3];
	int idx;

	idx = lsm6dso_shub_get_idx(SENSOR_CHAN_MAGN_XYZ);
	if (idx < 0) {
		LOG_DBG("external magn not supported");
		return -ENOTSUP;
	}


	sample[0] = sys_le16_to_cpu((s16_t)(data->ext_data[idx][0] |
				    (data->ext_data[idx][1] << 8)));
	sample[1] = sys_le16_to_cpu((s16_t)(data->ext_data[idx][2] |
				    (data->ext_data[idx][3] << 8)));
	sample[2] = sys_le16_to_cpu((s16_t)(data->ext_data[idx][4] |
				    (data->ext_data[idx][5] << 8)));

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		lsm6dso_magn_convert(val, sample[0],
				     data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_Y:
		lsm6dso_magn_convert(val, sample[1],
				     data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_Z:
		lsm6dso_magn_convert(val, sample[2],
				     data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		lsm6dso_magn_convert(val, sample[0],
				     data->magn_gain);
		lsm6dso_magn_convert(val + 1, sample[1],
				     data->magn_gain);
		lsm6dso_magn_convert(val + 2, sample[2],
				     data->magn_gain);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lsm6dso_hum_convert(struct sensor_value *val,
				       struct lsm6dso_data *data)
{
	float rh;
	s16_t raw_val;
	struct hts221_data *ht = &data->hts221;
	int idx;

	idx = lsm6dso_shub_get_idx(SENSOR_CHAN_HUMIDITY);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = sys_le16_to_cpu((s16_t)(data->ext_data[idx][0] |
					  (data->ext_data[idx][1] << 8)));

	/* find relative humidty by linear interpolation */
	rh = (ht->y1 - ht->y0) * raw_val + ht->x1 * ht->y0 - ht->x0 * ht->y1;
	rh /= (ht->x1 - ht->x0);

	/* convert humidity to integer and fractional part */
	val->val1 = rh;
	val->val2 = rh * 1000000;
}

static inline void lsm6dso_press_convert(struct sensor_value *val,
					 struct lsm6dso_data *data)
{
	s32_t raw_val;
	int idx;

	idx = lsm6dso_shub_get_idx(SENSOR_CHAN_PRESS);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = sys_le32_to_cpu((s32_t)(data->ext_data[idx][0] |
					  (data->ext_data[idx][1] << 8) |
					  (data->ext_data[idx][2] << 16)));

	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Convert raw_val to val in kPa */
	val->val1 = (raw_val >> 12) / 10;
	val->val2 = (raw_val >> 12) % 10 * 100000 +
		(((s32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void lsm6dso_temp_convert(struct sensor_value *val,
					struct lsm6dso_data *data)
{
	s16_t raw_val;
	int idx;

	idx = lsm6dso_shub_get_idx(SENSOR_CHAN_PRESS);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = sys_le16_to_cpu((s16_t)(data->ext_data[idx][3] |
					  (data->ext_data[idx][4] << 8)));

	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = (s32_t)raw_val % 100 * (10000);
}
#endif

static int lsm6dso_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lsm6dso_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dso_accel_channel_get(chan, val, data);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dso_gyro_channel_get(chan, val, data);
		break;
#if defined(CONFIG_LSM6DSO_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lsm6dso_gyro_channel_get_temp(val, data);
		break;
#endif
#if defined(CONFIG_LSM6DSO_SENSORHUB)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		lsm6dso_magn_get_channel(chan, val, data);
		break;

	case SENSOR_CHAN_HUMIDITY:
		lsm6dso_hum_convert(val, data);
		break;

	case SENSOR_CHAN_PRESS:
		lsm6dso_press_convert(val, data);
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		lsm6dso_temp_convert(val, data);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lsm6dso_api_funcs = {
	.attr_set = lsm6dso_attr_set,
#if CONFIG_LSM6DSO_TRIGGER
	.trigger_set = lsm6dso_trigger_set,
#endif
	.sample_fetch = lsm6dso_sample_fetch,
	.channel_get = lsm6dso_channel_get,
};

static int lsm6dso_init_chip(struct device *dev)
{
	struct lsm6dso_data *lsm6dso = dev->driver_data;
	u8_t chip_id;

	#if 0
	if (lsm6dso_reboot(dev) < 0) {
		LOG_DBG("failed to reboot device");
		return -EIO;
	}
	#endif

	if (lsm6dso->hw_tf->read_reg(lsm6dso, LSM6DSO_REG_WHO_AM_I,
				     &chip_id) < 0) {
		LOG_DBG("failed reading chip id");
		return -EIO;
	}
	if (chip_id != LSM6DSO_VAL_WHO_AM_I) {
		LOG_DBG("invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	LOG_INF("chip id 0x%x", chip_id);

	/* reset device */
	if (lsm6dso->hw_tf->write_reg(lsm6dso, LSM6DSO_REG_CTRL3_C,
				      LSM6DSO_MASK_CTRL3_C_SW_RESET)) {
		return -EIO;
	}

	k_busy_wait(100);

	if (lsm6dso_accel_set_fs_raw(dev,
				     LSM6DSO_DEFAULT_ACCEL_FULLSCALE) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}
	lsm6dso->acc_gain = LSM6DSO_DEFAULT_ACCEL_SENSITIVITY;

	lsm6dso->accel_freq = lsm6dso_odr_to_freq_val(CONFIG_LSM6DSO_ACCEL_ODR);
	if (lsm6dso_accel_set_odr_raw(dev, CONFIG_LSM6DSO_ACCEL_ODR) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	if (lsm6dso_gyro_set_fs_raw(dev, LSM6DSO_DEFAULT_GYRO_FULLSCALE) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}
	lsm6dso->gyro_gain = LSM6DSO_DEFAULT_GYRO_SENSITIVITY;

	lsm6dso->gyro_freq = lsm6dso_odr_to_freq_val(CONFIG_LSM6DSO_GYRO_ODR);
	if (lsm6dso_gyro_set_odr_raw(dev, CONFIG_LSM6DSO_GYRO_ODR) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	/* Set FIFO bypass mode */
	if (lsm6dso->hw_tf->update_reg(lsm6dso, LSM6DSO_REG_FIFO_CTRL4,
				       LSM6DSO_MASK_FIFO_CTRL4_FIFO_MODE,
				       0) < 0) {
		LOG_DBG("failed to set FIFO mode");
		return -EIO;
	}

	if (lsm6dso->hw_tf->update_reg(lsm6dso, LSM6DSO_REG_CTRL3_C,
				       LSM6DSO_MASK_CTRL3_C_BDU, 1)) {
		LOG_DBG("failed to set BDU, BLE and burst");
		return -EIO;
	}

	return 0;
}

static struct lsm6dso_config lsm6dso_config = {
	.bus_name = DT_ST_LSM6DSO_0_BUS_NAME,
#ifdef CONFIG_LSM6DSO_TRIGGER
	.int_gpio_port = DT_ST_LSM6DSO_0_IRQ_GPIOS_CONTROLLER,
	.int_gpio_pin = DT_ST_LSM6DSO_0_IRQ_GPIOS_PIN,
#if defined(CONFIG_LSM6DSO_INT_PIN_1)
	.int_pin = 1,
#elif defined(CONFIG_LSM6DSO_INT_PIN_2)
	.int_pin = 2,
#endif /* CONFIG_LSM6DSO_INT_PIN */

#endif /* CONFIG_LSM6DSO_TRIGGER */
};

static int lsm6dso_init(struct device *dev)
{
	const struct lsm6dso_config * const config = dev->config->config_info;
	struct lsm6dso_data *data = dev->driver_data;

	data->bus = device_get_binding(config->bus_name);
	if (!data->bus) {
		LOG_DBG("master not found: %s",
			    config->bus_name);
		return -EINVAL;
	}

#ifdef DT_ST_LSM6DSO_BUS_SPI
	lsm6dso_spi_init(dev);
#else
	lsm6dso_i2c_init(dev);
#endif

#ifdef CONFIG_LSM6DSO_TRIGGER
	if (lsm6dso_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	if (lsm6dso_init_chip(dev) < 0) {
		LOG_DBG("failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LSM6DSO_SENSORHUB
	if (lsm6dso_shub_init(dev) < 0) {
		LOG_DBG("failed to initialize external chip");
		return -EIO;
	}
#endif

	return 0;
}


static struct lsm6dso_data lsm6dso_data;

DEVICE_AND_API_INIT(lsm6dso, DT_ST_LSM6DSO_0_LABEL, lsm6dso_init,
		    &lsm6dso_data, &lsm6dso_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lsm6dso_api_funcs);
