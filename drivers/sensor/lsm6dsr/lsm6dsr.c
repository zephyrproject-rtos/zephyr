/* lsm6dsr.c - Driver for LSM6DSR accelerometer, gyroscope and
 * temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsr.pdf
 */

#define DT_DRV_COMPAT st_lsm6dsr

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include "lsm6dsr.h"

LOG_MODULE_REGISTER(LSM6DSR, CONFIG_SENSOR_LOG_LEVEL);

static const float lsm6dsr_odr_map[] = {
	0, 12.5, 26, 52, 104, 208, 417, 833, 1667, 3333, 6667, 1.6
};

static int lsm6dsr_freq_to_odr_val(const struct sensor_value *freq, size_t array_size)
{
	size_t i;

	for (i = 0; i < array_size; ++i) {
		if (sensor_value_to_float(freq) == lsm6dsr_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static float lsm6dsr_odr_to_freq_val(uint16_t odr, size_t array_size)
{
	/* for valid index, return value from map */
	if (odr < array_size) {
		return lsm6dsr_odr_map[odr];
	}

	/* invalid index, return last entry */
	return lsm6dsr_odr_map[array_size - 1];
}

static inline int lsm6dsr_reboot(const struct device *dev)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (lsm6dsr_boot_set(ctx, 1) < 0) {
		return -EIO;
	}

	/* Wait sensor turn-on time as per datasheet */
	k_busy_wait(35 * USEC_PER_MSEC);

	return 0;
}

static const uint16_t lsm6dsr_accel_fs_sens[] = {1, 8, 2, 4};

static int lsm6dsr_accel_set_fs_raw(const struct device *dev, lsm6dsr_fs_xl_t fs)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsr_data *data = dev->data;

	if (lsm6dsr_xl_full_scale_set(ctx, fs) < 0) {
		return -EIO;
	}

	data->acc_gain = lsm6dsr_accel_fs_sens[fs] * SENSI_GRAIN_XL;
	data->accel_fs = fs;

	return 0;
}

static size_t lsm6dsr_accel_get_odr_map_size(const struct device *dev)
{
	struct lsm6dsr_data *data = dev->data;

	if (data->accel_pm == LSM6DSR_LOW_NORMAL_POWER_MD) {
		return ARRAY_SIZE(lsm6dsr_odr_map); /* (low power only) */
	} else {
		return ARRAY_SIZE(lsm6dsr_odr_map) - 1;
	}
}

static int lsm6dsr_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsr_data *data = dev->data;
	size_t odr_map_size = lsm6dsr_accel_get_odr_map_size(dev);

	if (lsm6dsr_xl_data_rate_set(ctx, odr) < 0) {
		return -EIO;
	}

	data->accel_freq = lsm6dsr_odr_to_freq_val(odr, odr_map_size);

	return 0;
}

static int lsm6dsr_gyro_set_fs_raw(const struct device *dev, lsm6dsr_fs_g_t fs)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsr_data *data = dev->data;
	uint16_t gyro_gain;

	if (lsm6dsr_gy_full_scale_set(ctx, fs) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	switch (fs) {
	case 0:
		gyro_gain = 2;
		break;
	case 1:
		gyro_gain = 32;
		break;
	case 2:
		gyro_gain = 1;
		break;
	case 4:
		gyro_gain = 4;
		break;
	case 8:
		gyro_gain = 8;
		break;
	case 12:
		gyro_gain = 16;
		break;
	default:
		gyro_gain = 0;
		break;
	}
	data->gyro_gain = gyro_gain * SENSI_GRAIN_G;
	data->gyro_fs = fs;

	return 0;
}

static int lsm6dsr_gyro_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsr_data *data = dev->data;

	if (lsm6dsr_gy_data_rate_set(ctx, odr) < 0) {
		return -EIO;
	}

	data->gyro_freq = lsm6dsr_odr_to_freq_val(odr, ARRAY_SIZE(lsm6dsr_odr_map) - 1);

	return 0;
}

static int lsm6dsr_accel_odr_set(const struct device *dev, const struct sensor_value *freq)
{
	int odr;
	size_t odr_map_size = lsm6dsr_accel_get_odr_map_size(dev);

	odr = lsm6dsr_freq_to_odr_val(freq, odr_map_size);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dsr_accel_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}

static const uint16_t lsm6dsr_accel_fs_map[] = {2, 16, 4, 8};

static int lsm6dsr_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dsr_accel_fs_map); ++i) {
		if (range == lsm6dsr_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lsm6dsr_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;

	fs = lsm6dsr_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lsm6dsr_accel_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	return 0;
}

int lsm6dsr_accel_pm_set(const struct device *dev, uint8_t accel_pm)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsr_data *data = dev->data;
	lsm6dsr_xl_hm_mode_t mode;

	switch (accel_pm) {
	case 0: /* High Performance mode */
		mode = LSM6DSR_HIGH_PERFORMANCE_MD;
		break;
	case 1: /* Low/Normal Power mode */
		mode = LSM6DSR_LOW_NORMAL_POWER_MD;
		break;
	default:
		return -EINVAL;
	}
	if (lsm6dsr_xl_power_mode_set(ctx, mode) < 0) {
		return -EIO;
	}
	data->accel_pm = mode;

	return 0;
}

static int lsm6dsr_accel_config(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dsr_accel_range_set(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dsr_accel_odr_set(dev, val);
	case SENSOR_ATTR_CONFIGURATION:
		return lsm6dsr_accel_pm_set(dev, val->val1);
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsr_gyro_odr_set(const struct device *dev, const struct sensor_value *freq)
{
	int odr;

	odr = lsm6dsr_freq_to_odr_val(freq, ARRAY_SIZE(lsm6dsr_odr_map) - 1);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dsr_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	return 0;
}

static int lsm6dsr_gyro_range_set(const struct device *dev, int32_t range)
{
	lsm6dsr_fs_g_t fs;

	switch (range) {
	case 125:
		fs = LSM6DSR_125dps;
		break;
	case 250:
		fs = LSM6DSR_250dps;
		break;
	case 500:
		fs = LSM6DSR_500dps;
		break;
	case 1000:
		fs = LSM6DSR_1000dps;
		break;
	case 2000:
		fs = LSM6DSR_2000dps;
		break;
	case 4000:
		fs = LSM6DSR_4000dps;
		break;
	default:
		return -EINVAL;
	}

	if (lsm6dsr_gyro_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	return 0;
}

int lsm6dsr_gyro_pm_set(const struct device *dev, uint8_t gyro_pm)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsr_g_hm_mode_t mode;

	switch (gyro_pm) {
	case 0:
		mode = LSM6DSR_GY_HIGH_PERFORMANCE;
		break;
	case 1:
		mode = LSM6DSR_GY_NORMAL;
		break;
	default:
		return -EINVAL;
	}
	if (lsm6dsr_gy_power_mode_set(ctx, mode) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsr_gyro_config(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dsr_gyro_range_set(dev, sensor_rad_to_degrees(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dsr_gyro_odr_set(dev, val);
	case SENSOR_ATTR_CONFIGURATION:
		return lsm6dsr_gyro_pm_set(dev, val->val1);
	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsr_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lsm6dsr_accel_config(dev, chan, attr, val);
	case SENSOR_CHAN_GYRO_XYZ:
		return lsm6dsr_gyro_config(dev, chan, attr, val);
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsr_sample_fetch_accel(const struct device *dev)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsr_data *data = dev->data;

	if (lsm6dsr_acceleration_raw_get(ctx, data->acc) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	return 0;
}

static int lsm6dsr_sample_fetch_gyro(const struct device *dev)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsr_data *data = dev->data;

	if (lsm6dsr_angular_rate_raw_get(ctx, data->gyro) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	return 0;
}

static int lsm6dsr_sample_fetch_temp(const struct device *dev)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsr_data *data = dev->data;

	if (lsm6dsr_temperature_raw_get(ctx, &data->temp_sample) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	return 0;
}

static int lsm6dsr_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsr_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsr_sample_fetch_gyro(dev);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		if (IS_ENABLED(CONFIG_LSM6DSR_ENABLE_TEMP)) {
			lsm6dsr_sample_fetch_temp(dev);
		} else {
			return -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_ALL:
		lsm6dsr_sample_fetch_accel(dev);
		lsm6dsr_sample_fetch_gyro(dev);
		if (IS_ENABLED(CONFIG_LSM6DSR_ENABLE_TEMP)) {
			lsm6dsr_sample_fetch_temp(dev);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lsm6dsr_accel_convert(struct sensor_value *val, int raw_val,
					 uint32_t sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in ug/LSB */
	/* Convert to m/s^2 */
	dval = (int64_t)raw_val * sensitivity * SENSOR_G_DOUBLE;
	val->val1 = (int32_t)(dval / 1000000);
	val->val2 = (int32_t)(dval % 1000000);

}

static inline int lsm6dsr_accel_get_channel(enum sensor_channel chan,
					    struct sensor_value *val,
					    struct lsm6dsr_data *data,
					    uint32_t sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lsm6dsr_accel_convert(val, data->acc[0], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lsm6dsr_accel_convert(val, data->acc[1], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lsm6dsr_accel_convert(val, data->acc[2], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		for (i = 0; i < 3; ++i) {
			lsm6dsr_accel_convert(val++, data->acc[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsr_accel_channel_get(enum sensor_channel chan,
				     struct sensor_value *val,
				     struct lsm6dsr_data *data)
{
	return lsm6dsr_accel_get_channel(chan, val, data, data->acc_gain);
}

static inline void lsm6dsr_gyro_convert(struct sensor_value *val, int raw_val,
					uint32_t sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in udps/LSB */
	/* Convert to rad/s */
	dval = (int64_t)raw_val * sensitivity * SENSOR_DEG2RAD_DOUBLE;
	val->val1 = (int32_t)(dval / 1000000);
	val->val2 = (int32_t)(dval % 1000000);
}

static inline int lsm6dsr_gyro_get_channel(enum sensor_channel chan,
					   struct sensor_value *val,
					   struct lsm6dsr_data *data,
					   float sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		lsm6dsr_gyro_convert(val, data->gyro[0], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Y:
		lsm6dsr_gyro_convert(val, data->gyro[1], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Z:
		lsm6dsr_gyro_convert(val, data->gyro[2], sensitivity);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		for (i = 0; i < 3; ++i) {
			lsm6dsr_gyro_convert(val++, data->gyro[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsr_gyro_channel_get(enum sensor_channel chan,
				    struct sensor_value *val,
				    struct lsm6dsr_data *data)
{
	return lsm6dsr_gyro_get_channel(chan, val, data, data->gyro_gain);
}

static void lsm6dsr_gyro_channel_get_temp(struct sensor_value *val,
					  struct lsm6dsr_data *data)
{
	/* val = temp_sample / 256 + 25 */
	val->val1 = data->temp_sample / 256 + 25;
	val->val2 = (data->temp_sample % 256) * (1000000 / 256);
}

static int lsm6dsr_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lsm6dsr_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsr_accel_channel_get(chan, val, data);
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsr_gyro_channel_get(chan, val, data);
	case SENSOR_CHAN_DIE_TEMP:
		if (IS_ENABLED(CONFIG_LSM6DSR_ENABLE_TEMP)) {
			lsm6dsr_gyro_channel_get_temp(val, data);
		} else {
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lsm6dsr_driver_api = {
	.attr_set = lsm6dsr_attr_set,
#if CONFIG_LSM6DSR_TRIGGER
	.trigger_set = lsm6dsr_trigger_set,
#endif
	.sample_fetch = lsm6dsr_sample_fetch,
	.channel_get = lsm6dsr_channel_get,
};

static int lsm6dsr_init_chip(const struct device *dev)
{
	const struct lsm6dsr_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t chip_id;
	uint8_t odr, fs;

	if (lsm6dsr_reboot(dev) < 0) {
		LOG_DBG("failed to reboot device");
		return -EIO;
	}

	if (lsm6dsr_device_id_get(ctx, &chip_id) < 0) {
		LOG_DBG("failed reading chip id");
		return -EIO;
	}

	LOG_DBG("chip id 0x%x", chip_id);
	if (chip_id != LSM6DSR_ID) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/* I3C disable stay preserved after s/w reset */
	if (lsm6dsr_i3c_disable_set(ctx, LSM6DSR_I3C_DISABLE) < 0) {
		LOG_DBG("Failed to disable I3C");
		return -EIO;
	}

	/* set accel power mode */
	LOG_DBG("accel pm is %d", cfg->accel_pm);
	if (lsm6dsr_accel_pm_set(dev, cfg->accel_pm) < 0) {
		LOG_DBG("failed to set accelerometer mode");
		return -EIO;
	}

	fs = cfg->accel_range;
	LOG_DBG("accel range is %d", fs);
	if (lsm6dsr_accel_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set accelerometer range %d", fs);
		return -EIO;
	}

	odr = cfg->accel_odr;
	LOG_DBG("accel odr is %d", odr);
	if (lsm6dsr_accel_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set accelerometer %d", odr);
		return -EIO;
	}

	/* set gyro power mode */
	LOG_DBG("gyro pm is %d", cfg->gyro_pm);
	if (lsm6dsr_gyro_pm_set(dev, cfg->gyro_pm) < 0) {
		LOG_DBG("failed to set gyroscope mode");
		return -EIO;
	}

	fs = cfg->gyro_range;
	LOG_DBG("gyro range is %d", fs);
	if (lsm6dsr_gyro_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	odr = cfg->gyro_odr;
	LOG_DBG("gyro odr is %d", odr);
	if (lsm6dsr_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	/* Set FIFO bypass mode */
	if (lsm6dsr_fifo_mode_set(ctx, LSM6DSR_BYPASS_MODE) < 0) {
		LOG_DBG("failed to set FIFO mode");
		return -EIO;
	}

	if (lsm6dsr_block_data_update_set(ctx, 1) < 0) {
		LOG_DBG("failed to set BDU");
		return -EIO;
	}

	if (lsm6dsr_auto_increment_set(ctx, 1) < 0) {
		LOG_DBG("failed to set burst");
		return -EIO;
	}

	return 0;
}

static int lsm6dsr_init(const struct device *dev)
{
#ifdef CONFIG_LSM6DSR_TRIGGER
	const struct lsm6dsr_config * const cfg = dev->config;
#endif

	struct lsm6dsr_data *data = dev->data;

	LOG_INF("Initialize device %s", dev->name);
	data->dev = dev;

	if (lsm6dsr_init_chip(dev) < 0) {
		LOG_ERR("Failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LSM6DSR_TRIGGER
	if (cfg->trig_enabled) {
		if (lsm6dsr_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return -EIO;
		}
	}
#endif

	return 0;
}


#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LSM6DSR driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by LSM6DSR_DEFINE_SPI() and
 * LSM6DSR_DEFINE_I2C().
 */

#define LSM6DSR_DEVICE_INIT(inst)					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			    lsm6dsr_init,				\
			    NULL,					\
			    &lsm6dsr_data_##inst,			\
			    &lsm6dsr_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &lsm6dsr_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_LSM6DSR_TRIGGER
#define LSM6DSR_CFG_IRQ(inst) \
		.trig_enabled = true,						\
		.gpio_drdy = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),			\
		.int_pin = DT_INST_PROP(inst, int_pin)
#else
#define LSM6DSR_CFG_IRQ(inst)
#endif

#define LSM6DSR_CONFIG_COMMON(inst)					\
	.accel_pm = DT_INST_PROP(inst, accel_pm),			\
	.accel_odr = DT_INST_PROP(inst, accel_odr),			\
	.accel_range = DT_INST_PROP(inst, accel_range),		\
	.gyro_pm = DT_INST_PROP(inst, gyro_pm),				\
	.gyro_odr = DT_INST_PROP(inst, gyro_odr),			\
	.gyro_range = DT_INST_PROP(inst, gyro_range),			\
	.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed),                 \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),		\
		(LSM6DSR_CFG_IRQ(inst)), ())

#define LSM6DSR_SPI_OP  (SPI_WORD_SET(8) |				\
			 SPI_OP_MODE_MASTER |				\
			 SPI_MODE_CPOL |				\
			 SPI_MODE_CPHA)					\

#define LSM6DSR_CONFIG_SPI(inst)						\
	{								\
		STMEMSC_CTX_SPI(&lsm6dsr_config_##inst.stmemsc_cfg), \
		.stmemsc_cfg = {					\
			.spi = SPI_DT_SPEC_INST_GET(inst,		\
					   LSM6DSR_SPI_OP,		\
					   0),				\
		},							\
		LSM6DSR_CONFIG_COMMON(inst)				\
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LSM6DSR_CONFIG_I2C(inst)					\
	{								\
		STMEMSC_CTX_I2C(&lsm6dsr_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		},							\
		LSM6DSR_CONFIG_COMMON(inst)				\
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LSM6DSR_DEFINE(inst)				\
	static struct lsm6dsr_data lsm6dsr_data_##inst;			\
	static const struct lsm6dsr_config lsm6dsr_config_##inst =	\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
			(LSM6DSR_CONFIG_SPI(inst)),			\
			(LSM6DSR_CONFIG_I2C(inst)));			\
	LSM6DSR_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(LSM6DSR_DEFINE)
