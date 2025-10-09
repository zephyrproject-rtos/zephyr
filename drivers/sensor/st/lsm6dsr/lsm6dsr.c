/* lsm6dsr.c - Driver for LSM6DSR accelerometer, gyroscope and
 * temperature sensor
 */

/*
 * Copyright (c) 2025 Helbling Technik AG
 *
 * SPDX-License-Identifier: Apache-2.0
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

static const uint16_t lsm6dsr_odr_map[] = {0, 12, 26, 52, 104, 208, 416, 833, 1666, 3332, 6664, 1};

#if defined(LSM6DSR_ACCEL_ODR_RUNTIME) || defined(LSM6DSR_GYRO_ODR_RUNTIME)
static int lsm6dsr_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dsr_odr_map); i++) {
		if (freq == lsm6dsr_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static int lsm6dsr_odr_to_freq_val(uint16_t odr)
{
	/* for valid index, return value from map */
	if (odr < ARRAY_SIZE(lsm6dsr_odr_map)) {
		return lsm6dsr_odr_map[odr];
	}

	/* invalid index, return the fastest entry (6.66kHz) */
	BUILD_ASSERT(ARRAY_SIZE(lsm6dsr_odr_map) > 10);
	return lsm6dsr_odr_map[10];
}

#ifdef LSM6DSR_ACCEL_FS_RUNTIME
static const uint16_t lsm6dsr_accel_fs_map[] = {2, 16, 4, 8};
static const uint16_t lsm6dsr_accel_fs_sens[] = {1, 8, 2, 4};

static int lsm6dsr_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dsr_accel_fs_map); i++) {
		if (range == lsm6dsr_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

#ifdef LSM6DSR_GYRO_FS_RUNTIME
static const uint16_t lsm6dsr_gyro_fs_map[] = {250, 500, 1000, 2000, 125, 4000};
static const uint16_t lsm6dsr_gyro_fs_sens[] = {2, 4, 8, 16, 1, 32};

/* Indexes in maps of the two specially treated bits for gyro full scale */
#define GYRO_FULLSCALE_125  4
#define GYRO_FULLSCALE_4000 5

static int lsm6dsr_gyro_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lsm6dsr_gyro_fs_map); i++) {
		if (range == lsm6dsr_gyro_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

static inline int lsm6dsr_reboot(const struct device *dev)
{
	struct lsm6dsr_data *data = dev->data;

	if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL3_C, LSM6DSR_MASK_CTRL3_C_BOOT,
				    1 << LSM6DSR_SHIFT_CTRL3_C_BOOT) < 0) {
		return -EIO;
	}

	/* Wait sensor turn-on time as per datasheet */
	k_busy_wait(USEC_PER_MSEC * 35U);

	return 0;
}

static int lsm6dsr_accel_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct lsm6dsr_data *data = dev->data;

	if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL1_XL, LSM6DSR_MASK_CTRL1_XL_FS_XL,
				    fs << LSM6DSR_SHIFT_CTRL1_XL_FS_XL) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsr_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct lsm6dsr_data *data = dev->data;

	if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL1_XL, LSM6DSR_MASK_CTRL1_XL_ODR_XL,
				    odr << LSM6DSR_SHIFT_CTRL1_XL_ODR_XL) < 0) {
		return -EIO;
	}

	data->accel_freq = lsm6dsr_odr_to_freq_val(odr);

	return 0;
}

static int lsm6dsr_gyro_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct lsm6dsr_data *data = dev->data;

	switch (fs) {
	case GYRO_FULLSCALE_125: {
		if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL2_G,
					    LSM6DSR_MASK_CTRL2_FS4000 | LSM6DSR_MASK_CTRL2_FS125 |
						    LSM6DSR_MASK_CTRL2_G_FS_G,
					    1 << LSM6DSR_SHIFT_CTRL2_FS125) < 0) {
			return -EIO;
		}
		break;
	}
	case GYRO_FULLSCALE_4000: {
		if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL2_G,
					    LSM6DSR_MASK_CTRL2_FS4000 | LSM6DSR_MASK_CTRL2_FS125 |
						    LSM6DSR_MASK_CTRL2_G_FS_G,
					    1 << LSM6DSR_SHIFT_CTRL2_FS4000) < 0) {
			return -EIO;
		}
		break;
	}
	default: {
		if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL2_G,
					    LSM6DSR_MASK_CTRL2_FS125 | LSM6DSR_MASK_CTRL2_G_FS_G,
					    fs << LSM6DSR_SHIFT_CTRL2_G_FS_G) < 0) {
			return -EIO;
		}
		break;
	}
	}

	return 0;
}

static int lsm6dsr_gyro_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct lsm6dsr_data *data = dev->data;

	if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL2_G, LSM6DSR_MASK_CTRL2_G_ODR_G,
				    odr << LSM6DSR_SHIFT_CTRL2_G_ODR_G) < 0) {
		return -EIO;
	}

	data->gyro_freq = lsm6dsr_odr_to_freq_val(odr);

	return 0;
}

#ifdef LSM6DSR_ACCEL_ODR_RUNTIME
static int lsm6dsr_accel_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = lsm6dsr_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dsr_accel_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef LSM6DSR_ACCEL_FS_RUNTIME
static int lsm6dsr_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lsm6dsr_data *data = dev->data;

	fs = lsm6dsr_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lsm6dsr_accel_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	data->accel_sensitivity = (float)(lsm6dsr_accel_fs_sens[fs] * SENSI_GRAIN_XL);
	return 0;
}
#endif

static int lsm6dsr_accel_config(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
#ifdef LSM6DSR_ACCEL_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dsr_accel_range_set(dev, sensor_ms2_to_g(val));
#endif
#ifdef LSM6DSR_ACCEL_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dsr_accel_odr_set(dev, val->val1);
#endif
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

#ifdef LSM6DSR_GYRO_ODR_RUNTIME
static int lsm6dsr_gyro_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = lsm6dsr_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	if (lsm6dsr_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	return 0;
}
#endif

#ifdef LSM6DSR_GYRO_FS_RUNTIME
static int lsm6dsr_gyro_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lsm6dsr_data *data = dev->data;

	fs = lsm6dsr_gyro_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lsm6dsr_gyro_set_fs_raw(dev, fs) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	data->gyro_sensitivity = (float)(lsm6dsr_gyro_fs_sens[fs] * SENSI_GRAIN_G);
	return 0;
}
#endif

static int lsm6dsr_gyro_config(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
#ifdef LSM6DSR_GYRO_FS_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return lsm6dsr_gyro_range_set(dev, sensor_rad_to_degrees(val));
#endif
#ifdef LSM6DSR_GYRO_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm6dsr_gyro_odr_set(dev, val->val1);
#endif
	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsr_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
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
	struct lsm6dsr_data *data = dev->data;
	uint8_t buf[6];

	if (data->hw_tf->read_data(dev, LSM6DSR_REG_OUTX_L_XL, buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read accel sample");
		return -EIO;
	}

	data->accel_sample_x = (int16_t)((uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8));
	data->accel_sample_y = (int16_t)((uint16_t)(buf[2]) | ((uint16_t)(buf[3]) << 8));
	data->accel_sample_z = (int16_t)((uint16_t)(buf[4]) | ((uint16_t)(buf[5]) << 8));

	return 0;
}

static int lsm6dsr_sample_fetch_gyro(const struct device *dev)
{
	struct lsm6dsr_data *data = dev->data;
	uint8_t buf[6];

	if (data->hw_tf->read_data(dev, LSM6DSR_REG_OUTX_L_G, buf, sizeof(buf)) < 0) {
		LOG_DBG("failed to read gyro sample");
		return -EIO;
	}

	data->gyro_sample_x = (int16_t)((uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8));
	data->gyro_sample_y = (int16_t)((uint16_t)(buf[2]) | ((uint16_t)(buf[3]) << 8));
	data->gyro_sample_z = (int16_t)((uint16_t)(buf[4]) | ((uint16_t)(buf[5]) << 8));

	return 0;
}

static int lsm6dsr_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsr_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsr_sample_fetch_gyro(dev);
		break;
	case SENSOR_CHAN_ALL:
		lsm6dsr_sample_fetch_accel(dev);
		lsm6dsr_sample_fetch_gyro(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lsm6dsr_accel_convert(struct sensor_value *val, int raw_val, float sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in ug/LSB */
	/* Convert to m/s^2 */
	dval = (int64_t)raw_val * sensitivity;
	sensor_ug_to_ms2(dval, val);
}

static inline int lsm6dsr_accel_get_channel(enum sensor_channel chan, struct sensor_value *val,
					    struct lsm6dsr_data *data, float sensitivity)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lsm6dsr_accel_convert(val, data->accel_sample_x, sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lsm6dsr_accel_convert(val, data->accel_sample_y, sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lsm6dsr_accel_convert(val, data->accel_sample_z, sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsr_accel_convert(val, data->accel_sample_x, sensitivity);
		lsm6dsr_accel_convert(val + 1, data->accel_sample_y, sensitivity);
		lsm6dsr_accel_convert(val + 2, data->accel_sample_z, sensitivity);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsr_accel_channel_get(enum sensor_channel chan, struct sensor_value *val,
				     struct lsm6dsr_data *data)
{
	return lsm6dsr_accel_get_channel(chan, val, data, data->accel_sensitivity);
}

static inline void lsm6dsr_gyro_convert(struct sensor_value *val, int raw_val, float sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in udps/LSB */
	/* So, calculate value in 10 udps unit and then to rad/s */
	dval = (int64_t)raw_val * sensitivity / 10;
	sensor_10udegrees_to_rad(dval, val);
}

static inline int lsm6dsr_gyro_get_channel(enum sensor_channel chan, struct sensor_value *val,
					   struct lsm6dsr_data *data, float sensitivity)
{
	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		lsm6dsr_gyro_convert(val, data->gyro_sample_x, sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Y:
		lsm6dsr_gyro_convert(val, data->gyro_sample_y, sensitivity);
		break;
	case SENSOR_CHAN_GYRO_Z:
		lsm6dsr_gyro_convert(val, data->gyro_sample_z, sensitivity);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsr_gyro_convert(val, data->gyro_sample_x, sensitivity);
		lsm6dsr_gyro_convert(val + 1, data->gyro_sample_y, sensitivity);
		lsm6dsr_gyro_convert(val + 2, data->gyro_sample_z, sensitivity);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lsm6dsr_gyro_channel_get(enum sensor_channel chan, struct sensor_value *val,
				    struct lsm6dsr_data *data)
{
	return lsm6dsr_gyro_get_channel(chan, val, data, data->gyro_sensitivity);
}

static int lsm6dsr_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lsm6dsr_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		lsm6dsr_accel_channel_get(chan, val, data);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		lsm6dsr_gyro_channel_get(chan, val, data);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, lsm6dsr_driver_api) = {
	.attr_set = lsm6dsr_attr_set,
	.sample_fetch = lsm6dsr_sample_fetch,
	.channel_get = lsm6dsr_channel_get,
};

static int lsm6dsr_init_chip(const struct device *dev)
{
	struct lsm6dsr_data *data = dev->data;
	uint8_t chip_id;

	if (lsm6dsr_reboot(dev) < 0) {
		LOG_DBG("failed to reboot device");
		return -EIO;
	}

	if (data->hw_tf->read_reg(dev, LSM6DSR_REG_WHO_AM_I, &chip_id) < 0) {
		LOG_DBG("failed reading chip id");
		return -EIO;
	}
	if (chip_id != LSM6DSR_VAL_WHO_AM_I) {
		LOG_DBG("invalid chip id 0x%x (expected 0x%x)", chip_id, LSM6DSR_REG_WHO_AM_I);
		return -EIO;
	}

	LOG_DBG("chip id 0x%x", chip_id);

	if (lsm6dsr_accel_set_fs_raw(dev, LSM6DSR_DEFAULT_ACCEL_FULLSCALE) < 0) {
		LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}
	data->accel_sensitivity = LSM6DSR_DEFAULT_ACCEL_SENSITIVITY;

	if (lsm6dsr_accel_set_odr_raw(dev, CONFIG_LSM6DSR_ACCEL_ODR) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	if (lsm6dsr_gyro_set_fs_raw(dev, LSM6DSR_DEFAULT_GYRO_FULLSCALE) < 0) {
		LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}
	data->gyro_sensitivity = LSM6DSR_DEFAULT_GYRO_SENSITIVITY;

	if (lsm6dsr_gyro_set_odr_raw(dev, CONFIG_LSM6DSR_GYRO_ODR) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	/* Configure FIFO: Bypass mode --> FIFO disabled */
	if (data->hw_tf->update_reg(dev, LSM6DSR_REG_FIFO_CTRL4, LSM6DSR_MASK_FIFO_CTRL4_FIFO_MODE,
				    0 << LSM6DSR_SHIFT_FIFO_CTRL4_FIFO_MODE) < 0) {
		LOG_DBG("failed to set FIFO mode");
		return -EIO;
	}

	/* Configure:
	 * - BDU: Disable Block data update --> continuous update
	 * - Bit 1: Must be set to zero for the correct operation of the device
	 * - IF_INC: Enable auto increment during multiple byte access (burst)
	 */
	if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL3_C,
				    LSM6DSR_MASK_CTRL3_C_BDU | LSM6DSR_MASK_CTRL3_C_MUST_BE_ZERO |
					    LSM6DSR_MASK_CTRL3_C_IF_INC,
				    (1 << LSM6DSR_SHIFT_CTRL3_C_BDU) |
					    (0 << LSM6DSR_SHIFT_CTRL3_C_MUST_BE_ZERO) |
					    (1 << LSM6DSR_SHIFT_CTRL3_C_IF_INC)) < 0) {
		LOG_DBG("failed to set BDU, MUST_BE_ZERO and burst");
		return -EIO;
	}

	/* Configure:
	 * - Disable high performance operation mode for accelerometer
	 */
	if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL6_C, LSM6DSR_MASK_CTRL6_C_XL_HM_MODE,
				    (1 << LSM6DSR_SHIFT_CTRL6_C_XL_HM_MODE)) < 0) {
		LOG_DBG("failed to disable accelerometer high performance mode");
		return -EIO;
	}

	/* Configure:
	 * - Disable high performance operation mode for gyro
	 */
	if (data->hw_tf->update_reg(dev, LSM6DSR_REG_CTRL7_G, LSM6DSR_MASK_CTRL7_G_HM_MODE,
				    (1 << LSM6DSR_SHIFT_CTRL7_G_HM_MODE)) < 0) {
		LOG_DBG("failed to disable gyroscope high performance mode");
		return -EIO;
	}

	return 0;
}

static int lsm6dsr_init(const struct device *dev)
{
	int ret;
	const struct lsm6dsr_config *const config = dev->config;

	ret = config->bus_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize sensor bus");
		return ret;
	}

	ret = lsm6dsr_init_chip(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize chip");
		return ret;
	}

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LSM6DSR driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by LSM6DSR_DEFINE_SPI() and
 * LSM6DSR_DEFINE_I2C().
 */

#define LSM6DSR_DEVICE_INIT(inst)                                                                  \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lsm6dsr_init, NULL, &lsm6dsr_data_##inst,               \
				     &lsm6dsr_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &lsm6dsr_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */
#define LSM6DSR_CONFIG_SPI(inst)                                                                   \
	{                                                                                          \
		.bus_init = lsm6dsr_spi_init,                                                      \
		.bus_cfg.spi = SPI_DT_SPEC_INST_GET(                                               \
			inst,                                                                      \
			SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA, 0),  \
	}

#define LSM6DSR_DEFINE_SPI(inst)                                                                   \
	static struct lsm6dsr_data lsm6dsr_data_##inst;                                            \
	static const struct lsm6dsr_config lsm6dsr_config_##inst = LSM6DSR_CONFIG_SPI(inst);       \
	LSM6DSR_DEVICE_INIT(inst)

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LSM6DSR_CONFIG_I2C(inst)                                                                   \
	{                                                                                          \
		.bus_init = lsm6dsr_i2c_init,                                                      \
		.bus_cfg.i2c = I2C_DT_SPEC_INST_GET(inst),                                         \
	}

#define LSM6DSR_DEFINE_I2C(inst)                                                                   \
	static struct lsm6dsr_data lsm6dsr_data_##inst;                                            \
	static const struct lsm6dsr_config lsm6dsr_config_##inst = LSM6DSR_CONFIG_I2C(inst);       \
	LSM6DSR_DEVICE_INIT(inst)
/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LSM6DSR_DEFINE(inst)                                                                       \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (LSM6DSR_DEFINE_SPI(inst)),				\
		    (LSM6DSR_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(LSM6DSR_DEFINE)
