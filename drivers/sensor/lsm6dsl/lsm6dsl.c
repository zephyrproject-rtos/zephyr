/* lsm6dsl.c - Driver for LSM6DSL accelerometer, gyroscope and
 * temperature sensor
 */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#include "lsm6dsl.h"

static inline int lsm6dsl_reboot(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;

	if (data->hw_tf->update_reg(data, LSM6DSL_REG_CTRL3_C,
				    LSM6DSL_MASK_CTRL3_C_BOOT,
				    1 << LSM6DSL_SHIFT_CTRL3_C_BOOT) < 0) {
		return -EIO;
	}

	/* Wait sensor turn-on time as per datasheet */
	k_busy_wait(35 * USEC_PER_MSEC);

	return 0;
}

static int lsm6dsl_accel_set_fs_raw(struct device *dev, u8_t fs)
{
	struct lsm6dsl_data *data = dev->driver_data;

	if (data->hw_tf->update_reg(data,
				    LSM6DSL_REG_CTRL1_XL,
				    LSM6DSL_MASK_CTRL1_XL_FS_XL,
				    fs << LSM6DSL_SHIFT_CTRL1_XL_FS_XL) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_accel_set_odr_raw(struct device *dev, u8_t odr)
{
	struct lsm6dsl_data *data = dev->driver_data;

	if (data->hw_tf->update_reg(data,
				    LSM6DSL_REG_CTRL1_XL,
				    LSM6DSL_MASK_CTRL1_XL_ODR_XL,
				    odr << LSM6DSL_SHIFT_CTRL1_XL_ODR_XL) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_gyro_set_fs_raw(struct device *dev, u8_t fs)
{
	struct lsm6dsl_data *data = dev->driver_data;

	if (fs == GYRO_FULLSCALE_125) {
		if (data->hw_tf->update_reg(data,
					LSM6DSL_REG_CTRL2_G,
					LSM6DSL_MASK_CTRL2_FS125,
					1 << LSM6DSL_SHIFT_CTRL2_FS125) < 0) {
			return -EIO;
		}
	} else {
		if (data->hw_tf->update_reg(data,
					LSM6DSL_REG_CTRL2_G,
					LSM6DSL_MASK_CTRL2_G_FS_G,
					fs << LSM6DSL_SHIFT_CTRL2_G_FS_G) < 0) {
			return -EIO;
		}
	}

	return 0;
}

static int lsm6dsl_gyro_set_odr_raw(struct device *dev, u8_t odr)
{
	struct lsm6dsl_data *data = dev->driver_data;

	if (data->hw_tf->update_reg(data,
				    LSM6DSL_REG_CTRL2_G,
				    LSM6DSL_MASK_CTRL2_G_ODR_G,
				    odr << LSM6DSL_SHIFT_CTRL2_G_ODR_G) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_sample_fetch_accel(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;
	u8_t buf[6];

	if (data->hw_tf->read_data(data, LSM6DSL_REG_OUTX_L_XL,
				   buf, sizeof(buf)) < 0) {
		SYS_LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->accel_sample_x = (s16_t)((u16_t)(buf[0]) |
				((u16_t)(buf[1]) << 8));
	data->accel_sample_y = (s16_t)((u16_t)(buf[2]) |
				((u16_t)(buf[3]) << 8));
	data->accel_sample_z = (s16_t)((u16_t)(buf[4]) |
				((u16_t)(buf[5]) << 8));

	return 0;
}

static int lsm6dsl_sample_fetch_gyro(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;
	u8_t buf[6];

	if (data->hw_tf->read_data(data, LSM6DSL_REG_OUTX_L_G,
				   buf, sizeof(buf)) < 0) {
		SYS_LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->gyro_sample_x = (s16_t)((u16_t)(buf[0]) |
				((u16_t)(buf[1]) << 8));
	data->gyro_sample_y = (s16_t)((u16_t)(buf[2]) |
				((u16_t)(buf[3]) << 8));
	data->gyro_sample_z = (s16_t)((u16_t)(buf[4]) |
				((u16_t)(buf[5]) << 8));

	return 0;
}

#if defined(CONFIG_LSM6DSL_ENABLE_TEMP)
static int lsm6dsl_sample_fetch_temp(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;
	u8_t buf[2];

	if (data->hw_tf->read_data(data, LSM6DSL_REG_OUT_TEMP_L,
				   buf, sizeof(buf)) < 0) {
		SYS_LOG_DBG("failed to read sample");
		return -EIO;
	}

	data->temp_sample = (s16_t)((u16_t)(buf[0]) |
				((u16_t)(buf[1]) << 8));

	return 0;
}
#endif

#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
static int lsm6dsl_sample_fetch_magn(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;
	u8_t buf[6];

	if (lsm6dsl_shub_read_external_chip(dev, buf, sizeof(buf)) < 0) {
		SYS_LOG_DBG("failed to read ext mag sample");
		return -EIO;
	}

	data->magn_sample_x = (s16_t)((u16_t)(buf[0]) |
				((u16_t)(buf[1]) << 8));
	data->magn_sample_y = (s16_t)((u16_t)(buf[2]) |
				((u16_t)(buf[3]) << 8));
	data->magn_sample_z = (s16_t)((u16_t)(buf[4]) |
				((u16_t)(buf[5]) << 8));

	return 0;
}
#endif
#if defined(CONFIG_LSM6DSL_EXT0_LPS22HB)
static int lsm6dsl_sample_fetch_press(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;
	u8_t buf[5];

	if (lsm6dsl_shub_read_external_chip(dev, buf, sizeof(buf)) < 0) {
		SYS_LOG_DBG("failed to read ext press sample");
		return -EIO;
	}

	data->sample_press = (s32_t)((u32_t)(buf[0]) |
				     ((u32_t)(buf[1]) << 8) |
				     ((u32_t)(buf[2]) << 16));
	data->sample_temp = (s16_t)((u16_t)(buf[3]) |
				     ((u16_t)(buf[4]) << 8));

	return 0;
}
#endif

static int lsm6dsl_sample_fetch(struct device *dev, enum sensor_channel chan)
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
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
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
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
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
	double dval;

	/* Sensitivity is exposed in mg/LSB */
	/* Convert to m/s^2 */
	dval = (double)(raw_val) * sensitivity * SENSOR_G_DOUBLE / 1000;
	val->val1 = (s32_t)dval;
	val->val2 = (((s32_t)(dval * 1000)) % 1000) * 1000;

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
					LSM6DSL_DEFAULT_ACCEL_SENSITIVITY);
}

static inline void lsm6dsl_gyro_convert(struct sensor_value *val, int raw_val,
					float sensitivity)
{
	double dval;

	/* Sensitivity is exposed in mdps/LSB */
	/* Convert to rad/s */
	dval = (double)(raw_val * sensitivity * SENSOR_DEG2RAD_DOUBLE / 1000);
	val->val1 = (s32_t)dval;
	val->val2 = (((s32_t)(dval * 1000)) % 1000) * 1000;
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
					LSM6DSL_DEFAULT_GYRO_SENSITIVITY);
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

#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
static inline void lsm6dsl_magn_convert(struct sensor_value *val, int raw_val,
					float sensitivity)
{
	double dval;

	/* Sensitivity is exposed in mgauss/LSB */
	dval = (double)(raw_val * sensitivity);
	val->val1 = (s32_t)dval / 1000000;
	val->val2 = (s32_t)dval % 1000000;
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
					 s32_t raw_val)
{
	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Convert raw_val to val in kPa */
	val->val1 = (raw_val >> 12) / 10;
	val->val2 = (raw_val >> 12) % 10 * 100000 +
		(((s32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void lps22hb_temp_convert(struct sensor_value *val,
					s16_t raw_val)
{
	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = (s32_t)raw_val % 100 * (10000);
}
#endif

static int lsm6dsl_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lsm6dsl_data *data = dev->driver_data;

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
#if defined(CONFIG_LSM6DSL_EXT0_LIS2MDL)
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

static const struct sensor_driver_api lsm6dsl_api_funcs = {
#if CONFIG_LSM6DSL_TRIGGER
	.trigger_set = lsm6dsl_trigger_set,
#endif
	.sample_fetch = lsm6dsl_sample_fetch,
	.channel_get = lsm6dsl_channel_get,
};

static int lsm6dsl_init_chip(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;
	u8_t chip_id;

	if (lsm6dsl_reboot(dev) < 0) {
		SYS_LOG_DBG("failed to reboot device");
		return -EIO;
	}

	if (data->hw_tf->read_reg(data, LSM6DSL_REG_WHO_AM_I, &chip_id) < 0) {
		SYS_LOG_DBG("failed reading chip id");
		return -EIO;
	}
	if (chip_id != LSM6DSL_VAL_WHO_AM_I) {
		SYS_LOG_DBG("invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	SYS_LOG_DBG("chip id 0x%x", chip_id);

	if (lsm6dsl_accel_set_fs_raw(dev,
				LSM6DSL_DEFAULT_ACCEL_FULLSCALE) < 0) {
		SYS_LOG_DBG("failed to set accelerometer full-scale");
		return -EIO;
	}

	if (lsm6dsl_accel_set_odr_raw(dev,
				LSM6DSL_DEFAULT_ACCEL_SAMPLING_RATE) < 0) {
		SYS_LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	if (lsm6dsl_gyro_set_fs_raw(dev, LSM6DSL_DEFAULT_GYRO_FULLSCALE) < 0) {
		SYS_LOG_DBG("failed to set gyroscope full-scale");
		return -EIO;
	}

	if (lsm6dsl_gyro_set_odr_raw(dev,
				     LSM6DSL_DEFAULT_GYRO_SAMPLING_RATE) < 0) {
		SYS_LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	if (data->hw_tf->update_reg(data,
				LSM6DSL_REG_FIFO_CTRL5,
				LSM6DSL_MASK_FIFO_CTRL5_FIFO_MODE,
				0 << LSM6DSL_SHIFT_FIFO_CTRL5_FIFO_MODE) < 0) {
		SYS_LOG_DBG("failed to set FIFO mode");
		return -EIO;
	}

	if (data->hw_tf->update_reg(data,
				    LSM6DSL_REG_CTRL3_C,
				    LSM6DSL_MASK_CTRL3_C_BDU |
				    LSM6DSL_MASK_CTRL3_C_BLE |
				    LSM6DSL_MASK_CTRL3_C_IF_INC,
				    (1 << LSM6DSL_SHIFT_CTRL3_C_BDU) |
				    (0 << LSM6DSL_SHIFT_CTRL3_C_BLE) |
				    (1 << LSM6DSL_SHIFT_CTRL3_C_IF_INC)) < 0) {
		SYS_LOG_DBG("failed to set BDU, BLE and burst");
		return -EIO;
	}

	return 0;
}

static struct lsm6dsl_config lsm6dsl_config = {
#ifdef CONFIG_LSM6DSL_SPI
	.comm_master_dev_name = CONFIG_LSM6DSL_SPI_MASTER_DEV_NAME,
#else
	.comm_master_dev_name = CONFIG_LSM6DSL_I2C_MASTER_DEV_NAME,
#endif
};

static int lsm6dsl_init(struct device *dev)
{
	const struct lsm6dsl_config * const config = dev->config->config_info;
	struct lsm6dsl_data *data = dev->driver_data;

	data->comm_master = device_get_binding(config->comm_master_dev_name);
	if (!data->comm_master) {
		SYS_LOG_DBG("master not found: %s",
			    config->comm_master_dev_name);
		return -EINVAL;
	}

#ifdef CONFIG_LSM6DSL_SPI
	lsm6dsl_spi_init(dev);
#else
	lsm6dsl_i2c_init(dev);
#endif

#ifdef CONFIG_LSM6DSL_TRIGGER
	if (lsm6dsl_init_interrupt(dev) < 0) {
		SYS_LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	if (lsm6dsl_init_chip(dev) < 0) {
		SYS_LOG_DBG("failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LSM6DSL_SENSORHUB
	if (lsm6dsl_shub_init_external_chip(dev) < 0) {
		SYS_LOG_DBG("failed to initialize external chip");
		return -EIO;
	}
#endif

	return 0;
}


static struct lsm6dsl_data lsm6dsl_data;

DEVICE_AND_API_INIT(lsm6dsl, CONFIG_LSM6DSL_DEV_NAME, lsm6dsl_init,
		    &lsm6dsl_data, &lsm6dsl_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lsm6dsl_api_funcs);
