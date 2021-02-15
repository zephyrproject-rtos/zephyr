/* ST Microelectronics ISM330DHCX 6-axis IMU sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/ism330dhcx.pdf
 */

#define DT_DRV_COMPAT st_ism330dhcx

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "ism330dhcx.h"

LOG_MODULE_REGISTER(ISM330DHCX, CONFIG_SENSOR_LOG_LEVEL);

static const uint16_t ism330dhcx_odr_map[] = {0, 12, 26, 52, 104, 208, 416, 833,
					1660, 3330, 6660};

#if defined(ISM330DHCX_ACCEL_ODR_RUNTIME) || defined(ISM330DHCX_GYRO_ODR_RUNTIME)
static int ism330dhcx_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(ism330dhcx_odr_map); i++) {
		if (freq == ism330dhcx_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static int ism330dhcx_odr_to_freq_val(uint16_t odr)
{
	/* for valid index, return value from map */
	if (odr < ARRAY_SIZE(ism330dhcx_odr_map)) {
		return ism330dhcx_odr_map[odr];
	}

	/* invalid index, return last entry */
	return ism330dhcx_odr_map[ARRAY_SIZE(ism330dhcx_odr_map) - 1];
}

#ifdef ISM330DHCX_ACCEL_FS_RUNTIME
static const uint16_t ism330dhcx_accel_fs_map[] = {2, 16, 4, 8};
static const uint16_t ism330dhcx_accel_fs_sens[] = {1, 8, 2, 4};

static int ism330dhcx_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(ism330dhcx_accel_fs_map); i++) {
		if (range == ism330dhcx_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

#ifdef ISM330DHCX_GYRO_FS_RUNTIME
static const uint16_t ism330dhcx_gyro_fs_map[] = {250, 500, 1000, 2000, 125};
static const uint16_t ism330dhcx_gyro_fs_sens[] = {2, 4, 8, 16, 1};

static int ism330dhcx_gyro_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(ism330dhcx_gyro_fs_map); i++) {
		if (range == ism330dhcx_gyro_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static inline int ism330dhcx_reboot(const struct device *dev)
{
	struct ism330dhcx_data *data = dev->data;

	if (ism330dhcx_boot_set(data->ctx, 1) < 0) {
		return -EIO;
	}

	/* Wait sensor turn-on time as per datasheet */
	k_busy_wait(35 * USEC_PER_MSEC);

	return 0;
}

static int ism330dhcx_accel_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct ism330dhcx_data *data = dev->data;

	if (ism330dhcx_xl_full_scale_set(data->ctx, fs) < 0) {
		return -EIO;
	}

	data->accel_fs = fs;

	return 0;
}

static int ism330dhcx_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct ism330dhcx_data *data = dev->data;

	if (ism330dhcx_xl_data_rate_set(data->ctx, odr) < 0) {
		return -EIO;
	}

	data->accel_freq = ism330dhcx_odr_to_freq_val(odr);

	return 0;
}

static int ism330dhcx_gyro_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct ism330dhcx_data *data = dev->data;

	if (ism330dhcx_gy_full_scale_set(data->ctx, fs) < 0) {
		return -EIO;
	}

	return 0;
}

static int ism330dhcx_gyro_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct ism330dhcx_data *data = dev->data;

	if (ism330dhcx_gy_data_rate_set(data->ctx, odr) < 0) {
		return -EIO;
	}

	return 0;
}

#ifdef ISM330DHCX_ACCEL_ODR_RUNTIME
static int ism330dhcx_accel_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = ism330dhcx_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (ism330dhcx_accel_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef ISM330DHCX_ACCEL_FS_RUNTIME
static int ism330dhcx_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct ism330dhcx_data *data = dev->data;

	fs = ism330dhcx_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (ism330dhcx_accel_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	data->acc_gain = (ism330dhcx_accel_fs_sens[fs] * GAIN_UNIT_XL);
	return 0;
}
#endif

static int ism330dhcx_accel_config(const struct device *dev,
				   enum sensor_channel chan,
				   enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	switch (attr) {
#ifdef ISM330DHCX_ACCEL_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return ism330dhcx_accel_range_set(dev, sensor_ms2_to_g(val));
#endif
#ifdef ISM330DHCX_ACCEL_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return ism330dhcx_accel_odr_set(dev, val->val1);
#endif
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

#ifdef ISM330DHCX_GYRO_ODR_RUNTIME
static int ism330dhcx_gyro_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = ism330dhcx_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (ism330dhcx_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef ISM330DHCX_GYRO_FS_RUNTIME
static int ism330dhcx_gyro_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct ism330dhcx_data *data = dev->data;

	fs = ism330dhcx_gyro_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (ism330dhcx_gyro_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	data->gyro_gain = (ism330dhcx_gyro_fs_sens[fs] * GAIN_UNIT_G);
	return 0;
}
#endif

static int ism330dhcx_gyro_config(const struct device *dev,
				  enum sensor_channel chan,
				  enum sensor_attribute attr,
				  const struct sensor_value *val)
{
	switch (attr) {
#ifdef ISM330DHCX_GYRO_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return ism330dhcx_gyro_range_set(dev, sensor_rad_to_degrees(val));
#endif
#ifdef ISM330DHCX_GYRO_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return ism330dhcx_gyro_odr_set(dev, val->val1);
#endif
	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int ism330dhcx_attr_set(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return ism330dhcx_accel_config(dev, chan, attr, val);
	case SENSOR_CHAN_GYRO_XYZ:
		return ism330dhcx_gyro_config(dev, chan, attr, val);
#if defined(CONFIG_ISM330DHCX_SENSORHUB)
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_PRESS:
	case SENSOR_CHAN_HUMIDITY:
		return ism330dhcx_shub_config(dev, chan, attr, val);
#endif /* CONFIG_ISM330DHCX_SENSORHUB */
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int ism330dhcx_sample_fetch_accel(const struct device *dev)
{
	struct ism330dhcx_data *data = dev->data;
	int16_t buf[3];

	if (ism330dhcx_acceleration_raw_get(data->ctx, buf) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->acc[0] = sys_le16_to_cpu(buf[0]);
	data->acc[1] = sys_le16_to_cpu(buf[1]);
	data->acc[2] = sys_le16_to_cpu(buf[2]);

	return 0;
}

static int ism330dhcx_sample_fetch_gyro(const struct device *dev)
{
	struct ism330dhcx_data *data = dev->data;
	int16_t buf[3];

	if (ism330dhcx_angular_rate_raw_get(data->ctx, buf) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->gyro[0] = sys_le16_to_cpu(buf[0]);
	data->gyro[1] = sys_le16_to_cpu(buf[1]);
	data->gyro[2] = sys_le16_to_cpu(buf[2]);

	return 0;
}

#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
static int ism330dhcx_sample_fetch_temp(const struct device *dev)
{
	struct ism330dhcx_data *data = dev->data;
	int16_t buf;

	if (ism330dhcx_temperature_raw_get(data->ctx, &buf) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->temp_sample = sys_le16_to_cpu(buf);

	return 0;
}
#endif

#if defined(CONFIG_ISM330DHCX_SENSORHUB)
static int ism330dhcx_sample_fetch_shub(const struct device *dev)
{
	if (ism330dhcx_shub_fetch_external_devs(dev) < 0) {
		LOG_DBG("failed to read ext shub devices");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_ISM330DHCX_SENSORHUB */

static int ism330dhcx_sample_fetch(const struct device *dev,
				   enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		ism330dhcx_sample_fetch_accel(dev);
#if defined(CONFIG_ISM330DHCX_SENSORHUB)
		ism330dhcx_sample_fetch_shub(dev);
#endif
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		ism330dhcx_sample_fetch_gyro(dev);
		break;
#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		ism330dhcx_sample_fetch_temp(dev);
		break;
#endif
	case SENSOR_CHAN_ALL:
		ism330dhcx_sample_fetch_accel(dev);
		ism330dhcx_sample_fetch_gyro(dev);
#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
		ism330dhcx_sample_fetch_temp(dev);
#endif
#if defined(CONFIG_ISM330DHCX_SENSORHUB)
		ism330dhcx_sample_fetch_shub(dev);
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void ism330dhcx_accel_convert(struct sensor_value *val, int raw_val,
					    uint32_t sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in ug/LSB */
	/* Convert to m/s^2 */
	dval = (int64_t)(raw_val) * sensitivity * SENSOR_G_DOUBLE;
	val->val1 = (int32_t)(dval / 1000000);
	val->val2 = (int32_t)(dval % 1000000);

}

static inline int ism330dhcx_accel_get_channel(enum sensor_channel chan,
					       struct sensor_value *val,
					       struct ism330dhcx_data *data,
					       uint32_t sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		ism330dhcx_accel_convert(val, data->acc[0], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		ism330dhcx_accel_convert(val, data->acc[1], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		ism330dhcx_accel_convert(val, data->acc[2], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		for (i = 0; i < 3; i++) {
			ism330dhcx_accel_convert(val++, data->acc[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ism330dhcx_accel_channel_get(enum sensor_channel chan,
					struct sensor_value *val,
					struct ism330dhcx_data *data)
{
	return ism330dhcx_accel_get_channel(chan, val, data, data->acc_gain);
}

static inline void ism330dhcx_gyro_convert(struct sensor_value *val, int raw_val,
					   uint32_t sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in udps/LSB */
	/* Convert to rad/s */
	dval = (int64_t)(raw_val) * sensitivity * SENSOR_DEG2RAD_DOUBLE;
	val->val1 = (int32_t)(dval / 1000000);
	val->val2 = (int32_t)(dval % 1000000);
}

static inline int ism330dhcx_gyro_get_channel(enum sensor_channel chan,
					      struct sensor_value *val,
					      struct ism330dhcx_data *data,
					      uint32_t sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		ism330dhcx_gyro_convert(val, data->gyro[0], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Y:
		ism330dhcx_gyro_convert(val, data->gyro[1], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Z:
		ism330dhcx_gyro_convert(val, data->gyro[2], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		for (i = 0; i < 3; i++) {
			ism330dhcx_gyro_convert(val++, data->gyro[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ism330dhcx_gyro_channel_get(enum sensor_channel chan,
				       struct sensor_value *val,
				       struct ism330dhcx_data *data)
{
	return ism330dhcx_gyro_get_channel(chan, val, data,
					ISM330DHCX_DEFAULT_GYRO_SENSITIVITY);
}

#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
static void ism330dhcx_gyro_channel_get_temp(struct sensor_value *val,
					  struct ism330dhcx_data *data)
{
	/* val = temp_sample / 256 + 25 */
	val->val1 = data->temp_sample / 256 + 25;
	val->val2 = (data->temp_sample % 256) * (1000000 / 256);
}
#endif

#if defined(CONFIG_ISM330DHCX_SENSORHUB)
static inline void ism330dhcx_magn_convert(struct sensor_value *val, int raw_val,
					   uint16_t sensitivity)
{
	double dval;

	/* Sensitivity is exposed in mgauss/LSB */
	dval = (double)(raw_val * sensitivity);
	val->val1 = (int32_t)dval / 1000000;
	val->val2 = (int32_t)dval % 1000000;
}

static inline int ism330dhcx_magn_get_channel(enum sensor_channel chan,
					      struct sensor_value *val,
					      struct ism330dhcx_data *data)
{
	int16_t sample[3];
	int idx;

	idx = ism330dhcx_shub_get_idx(SENSOR_CHAN_MAGN_XYZ);
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
		ism330dhcx_magn_convert(val, sample[0], data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_Y:
		ism330dhcx_magn_convert(val, sample[1], data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_Z:
		ism330dhcx_magn_convert(val, sample[2], data->magn_gain);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		ism330dhcx_magn_convert(val, sample[0], data->magn_gain);
		ism330dhcx_magn_convert(val + 1, sample[1], data->magn_gain);
		ism330dhcx_magn_convert(val + 2, sample[2], data->magn_gain);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void ism330dhcx_hum_convert(struct sensor_value *val,
					  struct ism330dhcx_data *data)
{
	float rh;
	int16_t raw_val;
	struct hts221_data *ht = &data->hts221;
	int idx;

	idx = ism330dhcx_shub_get_idx(SENSOR_CHAN_HUMIDITY);
	if (idx < 0) {
		LOG_DBG("external press/temp not supported");
		return;
	}

	raw_val = ((int16_t)(data->ext_data[idx][0] |
			   (data->ext_data[idx][1] << 8)));

	/* find relative humidty by linear interpolation */
	rh = (ht->y1 - ht->y0) * raw_val + ht->x1 * ht->y0 - ht->x0 * ht->y1;
	rh /= (ht->x1 - ht->x0);

	/* convert humidity to integer and fractional part */
	val->val1 = rh;
	val->val2 = rh * 1000000;
}

static inline void ism330dhcx_press_convert(struct sensor_value *val,
					    struct ism330dhcx_data *data)
{
	int32_t raw_val;
	int idx;

	idx = ism330dhcx_shub_get_idx(SENSOR_CHAN_PRESS);
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

static inline void ism330dhcx_temp_convert(struct sensor_value *val,
					   struct ism330dhcx_data *data)
{
	int16_t raw_val;
	int idx;

	idx = ism330dhcx_shub_get_idx(SENSOR_CHAN_PRESS);
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

static int ism330dhcx_channel_get(const struct device *dev,
				  enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct ism330dhcx_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		ism330dhcx_accel_channel_get(chan, val, data);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		ism330dhcx_gyro_channel_get(chan, val, data);
		break;
#if defined(CONFIG_ISM330DHCX_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		ism330dhcx_gyro_channel_get_temp(val, data);
		break;
#endif
#if defined(CONFIG_ISM330DHCX_SENSORHUB)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		ism330dhcx_magn_get_channel(chan, val, data);
		break;

	case SENSOR_CHAN_HUMIDITY:
		ism330dhcx_hum_convert(val, data);
		break;

	case SENSOR_CHAN_PRESS:
		ism330dhcx_press_convert(val, data);
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		ism330dhcx_temp_convert(val, data);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api ism330dhcx_api_funcs = {
	.attr_set = ism330dhcx_attr_set,
#if CONFIG_ISM330DHCX_TRIGGER
	.trigger_set = ism330dhcx_trigger_set,
#endif
	.sample_fetch = ism330dhcx_sample_fetch,
	.channel_get = ism330dhcx_channel_get,
};

static int ism330dhcx_init_chip(const struct device *dev)
{
	struct ism330dhcx_data *ism330dhcx = dev->data;
	uint8_t chip_id;

	ism330dhcx->dev = dev;

	if (ism330dhcx_device_id_get(ism330dhcx->ctx, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		return -EIO;
	}

	LOG_INF("chip id 0x%x", chip_id);

	if (chip_id != ISM330DHCX_ID) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/* reset device */
	if (ism330dhcx_reset_set(ism330dhcx->ctx, 1) < 0) {
		return -EIO;
	}

	k_busy_wait(100);

	if (ism330dhcx_accel_set_fs_raw(dev,
				     ISM330DHCX_DEFAULT_ACCEL_FULLSCALE) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}
	ism330dhcx->acc_gain = ISM330DHCX_DEFAULT_ACCEL_SENSITIVITY;

	ism330dhcx->accel_freq = ism330dhcx_odr_to_freq_val(CONFIG_ISM330DHCX_ACCEL_ODR);
	if (ism330dhcx_accel_set_odr_raw(dev, CONFIG_ISM330DHCX_ACCEL_ODR) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	if (ism330dhcx_gyro_set_fs_raw(dev, ISM330DHCX_DEFAULT_GYRO_FULLSCALE) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}
	ism330dhcx->gyro_gain = ISM330DHCX_DEFAULT_GYRO_SENSITIVITY;

	ism330dhcx->gyro_freq = ism330dhcx_odr_to_freq_val(CONFIG_ISM330DHCX_GYRO_ODR);
	if (ism330dhcx_gyro_set_odr_raw(dev, CONFIG_ISM330DHCX_GYRO_ODR) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	/* Set FIFO bypass mode */
	if (ism330dhcx_fifo_mode_set(ism330dhcx->ctx, ISM330DHCX_BYPASS_MODE) < 0) {
		LOG_DBG("failed to set FIFO mode");
		return -EIO;
	}

	if (ism330dhcx_block_data_update_set(ism330dhcx->ctx, 1) < 0) {
		LOG_DBG("failed to set BDU mode");
		return -EIO;
	}

	return 0;
}

static struct ism330dhcx_data ism330dhcx_data;

static const struct ism330dhcx_config ism330dhcx_config = {
	.bus_name = DT_INST_BUS_LABEL(0),
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	.bus_init = ism330dhcx_spi_init,
	.spi_conf.frequency = DT_INST_PROP(0, spi_max_frequency),
	.spi_conf.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
			       SPI_MODE_CPHA | SPI_WORD_SET(8) |
			       SPI_LINES_SINGLE),
	.spi_conf.slave     = DT_INST_REG_ADDR(0),
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	.gpio_cs_port	    = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.cs_gpio	    = DT_INST_SPI_DEV_CS_GPIOS_PIN(0),
	.cs_gpio_flags	    = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0),

	.spi_conf.cs        =  &ism330dhcx_data.cs_ctrl,
#else
	.spi_conf.cs        = NULL,
#endif
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	.bus_init = ism330dhcx_i2c_init,
	.i2c_slv_addr = DT_INST_REG_ADDR(0),
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
#ifdef CONFIG_ISM330DHCX_TRIGGER
#if DT_INST_PROP_HAS_IDX(0, drdy_gpios, 1)
	/* Two gpio pins declared in DTS */
#if defined(CONFIG_ISM330DHCX_INT_PIN_1)
	.int_gpio_port = DT_INST_GPIO_LABEL_BY_IDX(0, drdy_gpios, 0),
	.int_gpio_pin = DT_INST_GPIO_PIN_BY_IDX(0, drdy_gpios, 0),
	.int_gpio_flags = DT_INST_GPIO_FLAGS_BY_IDX(0, drdy_gpios, 0),
	.int_pin = 1,
#elif defined(CONFIG_ISM330DHCX_INT_PIN_2)
	.int_gpio_port = DT_INST_GPIO_LABEL_BY_IDX(0, drdy_gpios, 1),
	.int_gpio_pin = DT_INST_GPIO_PIN_BY_IDX(0, drdy_gpios, 1),
	.int_gpio_flags = DT_INST_GPIO_FLAGS_BY_IDX(0, drdy_gpios, 1),
	.int_pin = 2,
#endif /* CONFIG_ISM330DHCX_INT_PIN_* */
#else
	/* One gpio pin declared in DTS */
	.int_gpio_port = DT_INST_GPIO_LABEL(0, drdy_gpios),
	.int_gpio_pin = DT_INST_GPIO_PIN(0, drdy_gpios),
	.int_gpio_flags = DT_INST_GPIO_FLAGS(0, drdy_gpios),
#if defined(CONFIG_ISM330DHCX_INT_PIN_1)
	.int_pin = 1,
#elif defined(CONFIG_ISM330DHCX_INT_PIN_2)
	.int_pin = 2,
#endif /* CONFIG_ISM330DHCX_INT_PIN_* */
#endif /* DT_INST_PROP_HAS_IDX(0, drdy_gpios, 1) */

#endif /* CONFIG_ISM330DHCX_TRIGGER */
};

static int ism330dhcx_init(const struct device *dev)
{
	const struct ism330dhcx_config * const config = dev->config;
	struct ism330dhcx_data *data = dev->data;

	data->bus = device_get_binding(config->bus_name);
	if (!data->bus) {
		LOG_DBG("master not found: %s",
			    config->bus_name);
		return -EINVAL;
	}

	config->bus_init(dev);

	if (ism330dhcx_init_chip(dev) < 0) {
		LOG_DBG("failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_ISM330DHCX_TRIGGER
	if (ism330dhcx_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

#ifdef CONFIG_ISM330DHCX_SENSORHUB
	if (ism330dhcx_shub_init(dev) < 0) {
		LOG_DBG("failed to initialize external chip");
		return -EIO;
	}
#endif

	return 0;
}


static struct ism330dhcx_data ism330dhcx_data;

DEVICE_DT_INST_DEFINE(0, ism330dhcx_init, device_pm_control_nop,
		    &ism330dhcx_data, &ism330dhcx_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &ism330dhcx_api_funcs);
