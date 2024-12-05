/*
 * Copyright (c) 2024 TDK Invensense
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/sensor/icm42670.h>
#include <zephyr/drivers/sensor/tdk_apex.h>

#include "icm42670.h"
#include "icm42670_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42670, CONFIG_SENSOR_LOG_LEVEL);

/* Convert DT enum to sensor ODR selection */
#define ICM42670_CONVERT_ENUM_TO_ODR_POS (4)

/* Maximum bytes to read/write on ICM42670 serial interface */
#define ICM42670_SERIAL_INTERFACE_MAX_READ  (1024 * 32)
#define ICM42670_SERIAL_INTERFACE_MAX_WRITE (1024 * 32)

static inline int icm42670_reg_read(const struct device *dev, uint8_t reg, uint8_t *buf,
				    uint32_t size)
{
	const struct icm42670_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, reg, buf, size);
}

static inline int inv_io_hal_read_reg(struct inv_imu_serif *serif, uint8_t reg, uint8_t *rbuffer,
				      uint32_t rlen)
{
	return icm42670_reg_read(serif->context, reg, rbuffer, rlen);
}

static inline int icm42670_reg_write(const struct device *dev, uint8_t reg, const uint8_t *buf,
				     uint32_t size)
{
	const struct icm42670_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, (uint8_t *)buf, size);
}

static inline int inv_io_hal_write_reg(struct inv_imu_serif *serif, uint8_t reg,
				       const uint8_t *wbuffer, uint32_t wlen)
{
	return icm42670_reg_write(serif->context, reg, wbuffer, wlen);
}

void inv_imu_sleep_us(uint32_t us)
{
	k_sleep(K_USEC(us));
}

uint64_t inv_imu_get_time_us(void)
{
	/* returns the elapsed time since the system booted, in milliseconds */
	return k_uptime_get() * 1000;
}

static uint16_t convert_dt_enum_to_freq(uint8_t val)
{
	uint16_t freq;

	switch (val) {
	case 0:
		freq = 0;
		break;
	case 1:
		freq = 1600;
		break;
	case 2:
		freq = 800;
		break;
	case 3:
		freq = 400;
		break;
	case 4:
		freq = 200;
		break;
	case 5:
		freq = 100;
		break;
	case 6:
		freq = 50;
		break;
	case 7:
		freq = 25;
		break;
	case 8:
		freq = 12;
		break;
	case 9:
		freq = 6;
		break;
	case 10:
		freq = 3;
		break;
	case 11:
		freq = 1;
		break;
	default:
		freq = 0;
		break;
	}
	return freq;
}

static uint32_t convert_freq_to_bitfield(uint32_t val, uint16_t *freq)
{
	uint32_t odr_bitfield = 0;

	if (val < 3 && val >= 1) {
		odr_bitfield = ACCEL_CONFIG0_ODR_1_5625_HZ; /*(= GYRO_CONFIG0_ODR_1_5625_HZ )*/
		*freq = 1;
	} else if (val < 6 && val >= 3) {
		odr_bitfield = ACCEL_CONFIG0_ODR_3_125_HZ; /*(= GYRO_CONFIG0_ODR_3_125_HZ )*/
		*freq = 3;
	} else if (val < 12 && val >= 6) {
		odr_bitfield = ACCEL_CONFIG0_ODR_6_25_HZ; /*(= GYRO_CONFIG0_ODR_6_25_HZ )*/
		*freq = 6;
	} else if (val < 25 && val >= 12) {
		odr_bitfield = ACCEL_CONFIG0_ODR_12_5_HZ; /*(= GYRO_CONFIG0_ODR_12_5_HZ )*/
		*freq = 12;
	} else if (val < 50 && val >= 25) {
		odr_bitfield = ACCEL_CONFIG0_ODR_25_HZ; /*(= GYRO_CONFIG0_ODR_25_HZ )*/
		*freq = 25;
	} else if (val < 100 && val >= 50) {
		odr_bitfield = ACCEL_CONFIG0_ODR_50_HZ; /*(GYRO_CONFIG0_ODR_50_HZ)*/
		*freq = 50;
	} else if (val < 200 && val >= 100) {
		odr_bitfield = ACCEL_CONFIG0_ODR_100_HZ; /*(= GYRO_CONFIG0_ODR_100_HZ )*/
		*freq = 100;
	} else if (val < 400 && val >= 200) {
		odr_bitfield = ACCEL_CONFIG0_ODR_200_HZ; /*(= GYRO_CONFIG0_ODR_200_HZ )*/
		*freq = 200;
	} else if (val < 800 && val >= 400) {
		odr_bitfield = ACCEL_CONFIG0_ODR_400_HZ; /*(= GYRO_CONFIG0_ODR_400_HZ )*/
		*freq = 400;
	} else if (val < 1600 && val >= 800) {
		odr_bitfield = ACCEL_CONFIG0_ODR_800_HZ; /*(= GYRO_CONFIG0_ODR_800_HZ )*/
		*freq = 800;
	} else if (val == 1600) {
		odr_bitfield = ACCEL_CONFIG0_ODR_1600_HZ; /*(= GYRO_CONFIG0_ODR_1600_HZ )*/
		*freq = 1600;
	}
	return odr_bitfield;
}

static uint32_t convert_acc_fs_to_bitfield(uint32_t val, uint8_t *fs)
{
	uint32_t bitfield = 0;

	if (val < 4 && val >= 2) {
		bitfield = ACCEL_CONFIG0_FS_SEL_2g;
		*fs = 2;
	} else if (val < 8 && val >= 4) {
		bitfield = ACCEL_CONFIG0_FS_SEL_4g;
		*fs = 4;
	} else if (val < 16 && val >= 8) {
		bitfield = ACCEL_CONFIG0_FS_SEL_8g;
		*fs = 8;
	} else if (val == 16) {
		bitfield = ACCEL_CONFIG0_FS_SEL_16g;
		*fs = 16;
	}
	return bitfield;
}

static uint32_t convert_gyr_fs_to_bitfield(uint32_t val, uint16_t *fs)
{
	uint32_t bitfield = 0;

	if (val < 500 && val >= 250) {
		bitfield = GYRO_CONFIG0_FS_SEL_250dps;
		*fs = 250;
	} else if (val < 1000 && val >= 500) {
		bitfield = GYRO_CONFIG0_FS_SEL_500dps;
		*fs = 500;
	} else if (val < 2000 && val >= 1000) {
		bitfield = GYRO_CONFIG0_FS_SEL_1000dps;
		*fs = 1000;
	} else if (val == 2000) {
		bitfield = GYRO_CONFIG0_FS_SEL_2000dps;
		*fs = 2000;
	}
	return bitfield;
}

static uint32_t convert_ln_bw_to_bitfield(uint32_t val)
{
	uint32_t bitfield = 0xFF;

	if (val < 25 && val >= 16) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_BW_16; /* (= GYRO_CONFIG1_GYRO_FILT_BW_16) */
	} else if (val < 34 && val >= 25) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_BW_25; /* (= GYRO_CONFIG1_GYRO_FILT_BW_25) */
	} else if (val < 53 && val >= 34) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_BW_34; /* (= GYRO_CONFIG1_GYRO_FILT_BW_34) */
	} else if (val < 73 && val >= 53) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_BW_53; /* (= GYRO_CONFIG1_GYRO_FILT_BW_53) */
	} else if (val < 121 && val >= 73) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_BW_73; /* (= GYRO_CONFIG1_GYRO_FILT_BW_73) */
	} else if (val < 180 && val >= 121) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_BW_121; /* (= GYRO_CONFIG1_GYRO_FILT_BW_121) */
	} else if (val == 180) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_BW_180; /* (= GYRO_CONFIG1_GYRO_FILT_BW_180) */
	} else if (val == 0) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_BW_NO_FILTER;
		/*(= GYRO_CONFIG1_GYRO_FILT_BW_NO_FILTER)*/
	}
	return bitfield;
}

static uint32_t convert_lp_avg_to_bitfield(uint32_t val)
{
	uint32_t bitfield = 0xFF;

	if (val < 4 && val >= 2) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_AVG_2;
	} else if (val < 8 && val >= 4) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_AVG_4;
	} else if (val < 16 && val >= 8) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_AVG_8;
	} else if (val < 32 && val >= 16) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_AVG_16;
	} else if (val < 64 && val >= 32) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_AVG_32;
	} else if (val == 64) {
		bitfield = ACCEL_CONFIG1_ACCEL_FILT_AVG_64;
	}
	return bitfield;
}

static uint8_t convert_bitfield_to_acc_fs(uint8_t bitfield)
{
	uint8_t acc_fs = 0;

	if (bitfield == ACCEL_CONFIG0_FS_SEL_2g) {
		acc_fs = 2;
	} else if (bitfield == ACCEL_CONFIG0_FS_SEL_4g) {
		acc_fs = 4;
	} else if (bitfield == ACCEL_CONFIG0_FS_SEL_8g) {
		acc_fs = 8;
	} else if (bitfield == ACCEL_CONFIG0_FS_SEL_16g) {
		acc_fs = 16;
	}
	return acc_fs;
}

static uint16_t convert_bitfield_to_gyr_fs(uint8_t bitfield)
{
	uint16_t gyr_fs = 0;

	if (bitfield == GYRO_CONFIG0_FS_SEL_250dps) {
		gyr_fs = 250;
	} else if (bitfield == GYRO_CONFIG0_FS_SEL_500dps) {
		gyr_fs = 500;
	} else if (bitfield == GYRO_CONFIG0_FS_SEL_1000dps) {
		gyr_fs = 1000;
	} else if (bitfield == GYRO_CONFIG0_FS_SEL_2000dps) {
		gyr_fs = 2000;
	}
	return gyr_fs;
}

static int icm42670_set_accel_power_mode(struct icm42670_data *drv_data,
					 const struct sensor_value *val)
{
	if ((val->val1 == ICM42670_LOW_POWER_MODE) &&
	    (drv_data->accel_pwr_mode != ICM42670_LOW_POWER_MODE)) {
		if (drv_data->accel_hz != 0) {
			if (drv_data->accel_hz <= 400) {
				inv_imu_enable_accel_low_power_mode(&drv_data->driver);
			} else {
				LOG_ERR("Not supported ATTR value");
				return -EINVAL;
			}
		}
		drv_data->accel_pwr_mode = val->val1;
	} else if ((val->val1 == ICM42670_LOW_NOISE_MODE) &&
		   (drv_data->accel_pwr_mode != ICM42670_LOW_NOISE_MODE)) {
		if (drv_data->accel_hz != 0) {
			if (drv_data->accel_hz >= 12) {
				inv_imu_enable_accel_low_noise_mode(&drv_data->driver);
			} else {
				LOG_ERR("Not supported ATTR value");
				return -EINVAL;
			}
		}
		drv_data->accel_pwr_mode = val->val1;
	} else {
		LOG_ERR("Not supported ATTR value");
		return -EINVAL;
	}
	return 0;
}

static int icm42670_set_accel_odr(struct icm42670_data *drv_data, const struct sensor_value *val)
{
	if (val->val1 <= 1600 && val->val1 >= 1) {
		if (drv_data->accel_hz == 0) {
			inv_imu_set_accel_frequency(
				&drv_data->driver,
				convert_freq_to_bitfield(val->val1, &drv_data->accel_hz));
			if (drv_data->accel_pwr_mode == ICM42670_LOW_POWER_MODE) {
				inv_imu_enable_accel_low_power_mode(&drv_data->driver);
			} else if (drv_data->accel_pwr_mode == ICM42670_LOW_NOISE_MODE) {
				inv_imu_enable_accel_low_noise_mode(&drv_data->driver);
			}
		} else {
			inv_imu_set_accel_frequency(
				&drv_data->driver,
				convert_freq_to_bitfield(val->val1, &drv_data->accel_hz));
		}
	} else if (val->val1 == 0) {
		inv_imu_disable_accel(&drv_data->driver);
		drv_data->accel_hz = val->val1;
	} else {
		LOG_ERR("Incorrect sampling value");
		return -EINVAL;
	}
	return 0;
}

static int icm42670_set_accel_fs(struct icm42670_data *drv_data, const struct sensor_value *val)
{
	if (val->val1 > 16 || val->val1 < 2) {
		LOG_ERR("Incorrect fullscale value");
		return -EINVAL;
	}
	inv_imu_set_accel_fsr(&drv_data->driver,
			      convert_acc_fs_to_bitfield(val->val1, &drv_data->accel_fs));
	LOG_DBG("Set accel full scale to: %d G", drv_data->accel_fs);
	return 0;
}

static int icm42670_accel_config(struct icm42670_data *drv_data, enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	if (attr == SENSOR_ATTR_CONFIGURATION) {
		icm42670_set_accel_power_mode(drv_data, val);

	} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		icm42670_set_accel_odr(drv_data, val);

	} else if (attr == SENSOR_ATTR_FULL_SCALE) {
		icm42670_set_accel_fs(drv_data, val);

	} else if ((enum sensor_attribute_icm42670)attr == SENSOR_ATTR_BW_FILTER_LPF) {
		if (val->val1 > 180) {
			LOG_ERR("Incorrect low pass filter bandwidth value");
			return -EINVAL;
		}
		inv_imu_set_accel_ln_bw(&drv_data->driver, convert_ln_bw_to_bitfield(val->val1));

	} else if ((enum sensor_attribute_icm42670)attr == SENSOR_ATTR_AVERAGING) {
		if (val->val1 > 64 || val->val1 < 2) {
			LOG_ERR("Incorrect averaging filter value");
			return -EINVAL;
		}
		inv_imu_set_accel_lp_avg(&drv_data->driver, convert_lp_avg_to_bitfield(val->val1));
	} else {
		LOG_ERR("Unsupported attribute");
		return -EINVAL;
	}
	return 0;
}

static int icm42670_set_gyro_odr(struct icm42670_data *drv_data, const struct sensor_value *val)
{
	if (val->val1 <= 1600 && val->val1 > 12) {
		if (drv_data->gyro_hz == 0) {
			inv_imu_set_gyro_frequency(
				&drv_data->driver,
				convert_freq_to_bitfield(val->val1, &drv_data->gyro_hz));
			inv_imu_enable_gyro_low_noise_mode(&drv_data->driver);
		} else {
			inv_imu_set_gyro_frequency(
				&drv_data->driver,
				convert_freq_to_bitfield(val->val1, &drv_data->gyro_hz));
		}
	} else if (val->val1 == 0) {
		inv_imu_disable_gyro(&drv_data->driver);
		drv_data->gyro_hz = val->val1;
	} else {
		LOG_ERR("Incorrect sampling value");
		return -EINVAL;
	}
	return 0;
}

static int icm42670_set_gyro_fs(struct icm42670_data *drv_data, const struct sensor_value *val)
{
	if (val->val1 > 2000 || val->val1 < 250) {
		LOG_ERR("Incorrect fullscale value");
		return -EINVAL;
	}
	inv_imu_set_gyro_fsr(&drv_data->driver,
			     convert_gyr_fs_to_bitfield(val->val1, &drv_data->gyro_fs));
	LOG_DBG("Set gyro fullscale to: %d dps", drv_data->gyro_fs);
	return 0;
}

static int icm42670_gyro_config(struct icm42670_data *drv_data, enum sensor_attribute attr,
				const struct sensor_value *val)
{
	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		icm42670_set_gyro_odr(drv_data, val);

	} else if (attr == SENSOR_ATTR_FULL_SCALE) {
		icm42670_set_gyro_fs(drv_data, val);

	} else if ((enum sensor_attribute_icm42670)attr == SENSOR_ATTR_BW_FILTER_LPF) {
		if (val->val1 > 180) {
			LOG_ERR("Incorrect low pass filter bandwidth value");
			return -EINVAL;
		}
		inv_imu_set_gyro_ln_bw(&drv_data->driver, convert_ln_bw_to_bitfield(val->val1));

	} else {
		LOG_ERR("Unsupported attribute");
		return -EINVAL;
	}
	return 0;
}

static int icm42670_sensor_init(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	int err = 0;

	/* Initialize serial interface and device */
	data->serif.context = (struct device *)dev;
	data->serif.read_reg = inv_io_hal_read_reg;
	data->serif.write_reg = inv_io_hal_write_reg;
	data->serif.max_read = ICM42670_SERIAL_INTERFACE_MAX_READ;
	data->serif.max_write = ICM42670_SERIAL_INTERFACE_MAX_WRITE;
#if CONFIG_SPI
	data->serif.serif_type = UI_SPI4;
#endif
#if CONFIG_I2C
	data->serif.serif_type = UI_I2C;
#endif
	err = inv_imu_init(&data->driver, &data->serif, NULL);
	if (err < 0) {
		LOG_ERR("Init failed: %d", err);
		return err;
	}

	err = inv_imu_get_who_am_i(&data->driver, &data->chip_id);
	if (err < 0) {
		LOG_ERR("ID read failed: %d", err);
		return err;
	}

	if (data->chip_id != data->imu_whoami) {
		LOG_ERR("invalid WHO_AM_I value, was 0x%x but expected 0x%x for %s", data->chip_id,
			data->imu_whoami, data->imu_name);
		return -ENOTSUP;
	}

	LOG_DBG("\"%s\" %s OK", dev->name, data->imu_name);
	return 0;
}

static int icm42670_turn_on_sensor(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	const struct icm42670_config *cfg = dev->config;
	int err = 0;

	err = inv_imu_set_accel_fsr(&data->driver,
				    (cfg->accel_fs << ACCEL_CONFIG0_ACCEL_UI_FS_SEL_POS));
	data->accel_fs =
		convert_bitfield_to_acc_fs((cfg->accel_fs << ACCEL_CONFIG0_ACCEL_UI_FS_SEL_POS));
	if ((err < 0) || (data->accel_fs == 0)) {
		LOG_ERR("Failed to configure accel FSR");
		return -EIO;
	}
	LOG_DBG("Set accel full scale to: %d G", data->accel_fs);

	err = inv_imu_set_gyro_fsr(&data->driver,
				   (cfg->gyro_fs << GYRO_CONFIG0_GYRO_UI_FS_SEL_POS));
	data->gyro_fs =
		convert_bitfield_to_gyr_fs((cfg->gyro_fs << GYRO_CONFIG0_GYRO_UI_FS_SEL_POS));
	if ((err < 0) || (data->gyro_fs == 0)) {
		LOG_ERR("Failed to configure gyro FSR");
		return -EIO;
	}
	LOG_DBG("Set gyro full scale to: %d dps", data->gyro_fs);

	err = inv_imu_set_accel_lp_avg(&data->driver,
				       (cfg->accel_avg << ACCEL_CONFIG1_ACCEL_UI_AVG_POS));
	err |= inv_imu_set_accel_ln_bw(&data->driver,
				       (cfg->accel_filt_bw << ACCEL_CONFIG1_ACCEL_UI_FILT_BW_POS));
	err |= inv_imu_set_gyro_ln_bw(&data->driver,
				      (cfg->gyro_filt_bw << GYRO_CONFIG1_GYRO_UI_FILT_BW_POS));
	if (err < 0) {
		LOG_ERR("Failed to configure filtering.");
		return -EIO;
	}

	if (cfg->accel_hz != 0) {
		err |= inv_imu_set_accel_frequency(
			&data->driver, cfg->accel_hz + ICM42670_CONVERT_ENUM_TO_ODR_POS);
		if ((cfg->accel_pwr_mode == ICM42670_LOW_NOISE_MODE) &&
		    (convert_dt_enum_to_freq(cfg->accel_hz) >= 12)) {
			err |= inv_imu_enable_accel_low_noise_mode(&data->driver);
		} else if ((cfg->accel_pwr_mode == ICM42670_LOW_POWER_MODE) &&
			   (convert_dt_enum_to_freq(cfg->accel_hz) <= 400)) {
			err |= inv_imu_enable_accel_low_power_mode(&data->driver);
		} else {
			LOG_ERR("Not supported power mode value");
		}
	}
	if (cfg->gyro_hz != 0) {
		err |= inv_imu_set_gyro_frequency(&data->driver,
						  cfg->gyro_hz + ICM42670_CONVERT_ENUM_TO_ODR_POS);
		err |= inv_imu_enable_gyro_low_noise_mode(&data->driver);
	}

	if (err < 0) {
		LOG_ERR("Failed to configure ODR.");
		return -EIO;
	}

	data->accel_pwr_mode = cfg->accel_pwr_mode;
	data->accel_hz = convert_dt_enum_to_freq(cfg->accel_hz);
	data->gyro_hz = convert_dt_enum_to_freq(cfg->gyro_hz);

	/*
	 * Accelerometer sensor need at least 10ms startup time
	 * Gyroscope sensor need at least 30ms startup time
	 */
	k_msleep(100);

	return 0;
}

static void icm42670_convert_accel(struct sensor_value *val, int16_t raw_val, uint16_t fs)
{
	int64_t conv_val;

	/* 16 bit accelerometer. 2^15 bits represent the range in G */
	/* see datasheet section 3.2 for details */
	conv_val = (int64_t)raw_val * SENSOR_G * fs / INT16_MAX;

	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

static void icm42670_convert_gyro(struct sensor_value *val, int16_t raw_val, uint16_t fs)
{
	int64_t conv_val;

	/* 16 bit gyroscope. 2^15 bits represent the range in degrees/s */
	/* see datasheet section 3.1 for details */
	conv_val = ((int64_t)raw_val * fs * SENSOR_PI) / (INT16_MAX * 180U);

	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

static void icm42670_convert_temp(struct sensor_value *val, int16_t raw_val)
{
	int64_t conv_val;

	/* see datasheet section 15.9 for details */
	conv_val = 25 * 1000000 + ((int64_t)raw_val * 1000000 / 2);
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

static int icm42670_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	int res = 0;
	struct icm42670_data *data = dev->data;
#ifdef CONFIG_TDK_APEX
	const struct icm42670_config *cfg = dev->config;
#endif

	icm42670_lock(dev);

	if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		icm42670_convert_accel(&val[0], data->accel_x, data->accel_fs);
		icm42670_convert_accel(&val[1], data->accel_y, data->accel_fs);
		icm42670_convert_accel(&val[2], data->accel_z, data->accel_fs);
	} else if (chan == SENSOR_CHAN_ACCEL_X) {
		icm42670_convert_accel(val, data->accel_x, data->accel_fs);
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		icm42670_convert_accel(val, data->accel_y, data->accel_fs);
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		icm42670_convert_accel(val, data->accel_z, data->accel_fs);
	} else if (chan == SENSOR_CHAN_GYRO_XYZ) {
		icm42670_convert_gyro(&val[0], data->gyro_x, data->gyro_fs);
		icm42670_convert_gyro(&val[1], data->gyro_y, data->gyro_fs);
		icm42670_convert_gyro(&val[2], data->gyro_z, data->gyro_fs);
	} else if (chan == SENSOR_CHAN_GYRO_X) {
		icm42670_convert_gyro(val, data->gyro_x, data->gyro_fs);
	} else if (chan == SENSOR_CHAN_GYRO_Y) {
		icm42670_convert_gyro(val, data->gyro_y, data->gyro_fs);
	} else if (chan == SENSOR_CHAN_GYRO_Z) {
		icm42670_convert_gyro(val, data->gyro_z, data->gyro_fs);
	} else if (chan == SENSOR_CHAN_DIE_TEMP) {
		icm42670_convert_temp(val, data->temp);
#ifdef CONFIG_TDK_APEX
	} else if ((enum sensor_channel_tdk_apex)chan == SENSOR_CHAN_APEX_MOTION) {
		if (cfg->apex == TDK_APEX_PEDOMETER) {
			val[0].val1 = data->pedometer_cnt;
			val[1].val1 = data->pedometer_activity;
			icm42670_apex_pedometer_cadence_convert(&val[2], data->pedometer_cadence,
								data->dmp_odr_hz);
		} else if (cfg->apex == TDK_APEX_WOM) {
			val[0].val1 = (data->apex_status & ICM42670_APEX_STATUS_MASK_WOM_X) ? 1 : 0;
			val[1].val1 = (data->apex_status & ICM42670_APEX_STATUS_MASK_WOM_Y) ? 1 : 0;
			val[2].val1 = (data->apex_status & ICM42670_APEX_STATUS_MASK_WOM_Z) ? 1 : 0;
		} else if ((cfg->apex == TDK_APEX_TILT) || (cfg->apex == TDK_APEX_SMD)) {
			val[0].val1 = data->apex_status;
		}
#endif
	} else {
		res = -ENOTSUP;
	}

	icm42670_unlock(dev);

	return res;
}

#ifdef CONFIG_ICM42670_TRIGGER
static int icm42670_fetch_from_fifo(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	int status = 0;
	uint8_t int_status;
	uint16_t packet_size = FIFO_HEADER_SIZE + FIFO_ACCEL_DATA_SIZE + FIFO_GYRO_DATA_SIZE +
			       FIFO_TEMP_DATA_SIZE + FIFO_TS_FSYNC_SIZE;
	uint16_t fifo_idx = 0;

	/* Ensure data ready status bit is set */
	status |= inv_imu_read_reg(&data->driver, INT_STATUS, 1, &int_status);
	if (status != 0) {
		return status;
	}

	if ((int_status & INT_STATUS_FIFO_THS_INT_MASK) ||
	    (int_status & INT_STATUS_FIFO_FULL_INT_MASK)) {
		uint16_t packet_count;

		/* Make sure RCOSC is enabled to guarrantee FIFO read */
		status |= inv_imu_switch_on_mclk(&data->driver);

		/* Read FIFO frame count */
		status |= inv_imu_get_frame_count(&data->driver, &packet_count);

		/* Check for error */
		if (status != 0) {
			status |= inv_imu_switch_off_mclk(&data->driver);
			return status;
		}

		/* Read FIFO data */
		status |= inv_imu_read_reg(&data->driver, FIFO_DATA, packet_size * packet_count,
					   (uint8_t *)&data->driver.fifo_data);

		/* Check for error */
		if (status != 0) {
			status |= inv_imu_reset_fifo(&data->driver);
			status |= inv_imu_switch_off_mclk(&data->driver);
			return status;
		}

		for (uint16_t i = 0; i < packet_count; i++) {
			inv_imu_sensor_event_t event;

			status |= inv_imu_decode_fifo_frame(
				&data->driver, &data->driver.fifo_data[fifo_idx], &event);
			fifo_idx += packet_size;

			/* Check for error */
			if (status != 0) {
				status |= inv_imu_reset_fifo(&data->driver);
				status |= inv_imu_switch_off_mclk(&data->driver);
				return status;
			}

			if (event.sensor_mask & (1 << INV_SENSOR_ACCEL)) {
				data->accel_x = event.accel[0];
				data->accel_y = event.accel[1];
				data->accel_z = event.accel[2];
			}
			if (event.sensor_mask & (1 << INV_SENSOR_GYRO)) {
				data->gyro_x = event.gyro[0];
				data->gyro_y = event.gyro[1];
				data->gyro_z = event.gyro[2];
			}
			if (event.sensor_mask & (1 << INV_SENSOR_TEMPERATURE)) {
				data->temp = event.temperature;
			}
			/*
			 * TODO use the sensor streaming interface with RTIO to handle multiple
			 * samples in FIFO
			 */

		} /* end of FIFO read for loop */

		status |= inv_imu_switch_off_mclk(&data->driver);
		if (status < 0) {
			return status;
		}
	} /*else: FIFO threshold was not reached and FIFO was not full */

	return 0;
}
#endif

#ifndef CONFIG_ICM42670_TRIGGER
static int icm42670_sample_fetch_accel(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	uint8_t buffer[ACCEL_DATA_SIZE];

	int res = inv_imu_read_reg(&data->driver, ACCEL_DATA_X1, ACCEL_DATA_SIZE, buffer);

	if (res) {
		return res;
	}

	data->accel_x = (int16_t)sys_get_be16(&buffer[0]);
	data->accel_y = (int16_t)sys_get_be16(&buffer[2]);
	data->accel_z = (int16_t)sys_get_be16(&buffer[4]);

	return 0;
}

static int icm42670_sample_fetch_gyro(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	uint8_t buffer[GYRO_DATA_SIZE];

	int res = inv_imu_read_reg(&data->driver, GYRO_DATA_X1, GYRO_DATA_SIZE, buffer);

	if (res) {
		return res;
	}

	data->gyro_x = (int16_t)sys_get_be16(&buffer[0]);
	data->gyro_y = (int16_t)sys_get_be16(&buffer[2]);
	data->gyro_z = (int16_t)sys_get_be16(&buffer[4]);

	return 0;
}

static int icm42670_sample_fetch_temp(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	uint8_t buffer[TEMP_DATA_SIZE];

	int res = inv_imu_read_reg(&data->driver, TEMP_DATA1, TEMP_DATA_SIZE, buffer);

	if (res) {
		return res;
	}

	data->temp = (int16_t)sys_get_be16(&buffer[0]);

	return 0;
}

static int icm42670_fetch_from_registers(const struct device *dev, enum sensor_channel chan)
{
	struct icm42670_data *data = dev->data;
	int res = 0;
	uint8_t int_status;

	LOG_ERR("Fetch from reg");

	icm42670_lock(dev);

	/* Ensure data ready status bit is set */
	int err = inv_imu_read_reg(&data->driver, INT_STATUS_DRDY, 1, &int_status);

	if (int_status & INT_STATUS_DRDY_DATA_RDY_INT_MASK) {
		switch (chan) {
		case SENSOR_CHAN_ALL:
			err |= icm42670_sample_fetch_accel(dev);
			err |= icm42670_sample_fetch_gyro(dev);
			err |= icm42670_sample_fetch_temp(dev);
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
			err |= icm42670_sample_fetch_accel(dev);
			break;
		case SENSOR_CHAN_GYRO_XYZ:
		case SENSOR_CHAN_GYRO_X:
		case SENSOR_CHAN_GYRO_Y:
		case SENSOR_CHAN_GYRO_Z:
			err |= icm42670_sample_fetch_gyro(dev);
			break;
		case SENSOR_CHAN_DIE_TEMP:
			err |= icm42670_sample_fetch_temp(dev);
			break;
		default:
			res = -ENOTSUP;
			break;
		}
	}

	icm42670_unlock(dev);

	if (err < 0) {
		res = -EIO;
	}
	return res;
}
#endif

static int icm42670_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int status = -ENOTSUP;

	icm42670_lock(dev);

#ifdef CONFIG_TDK_APEX
	if ((enum sensor_channel_tdk_apex)chan == SENSOR_CHAN_APEX_MOTION) {
		status = icm42670_apex_fetch_from_dmp(dev);
	}
#endif

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_ACCEL_XYZ) ||
	    (chan == SENSOR_CHAN_ACCEL_X) || (chan == SENSOR_CHAN_ACCEL_Y) ||
	    (chan == SENSOR_CHAN_ACCEL_Z) || (chan == SENSOR_CHAN_GYRO_XYZ) ||
	    (chan == SENSOR_CHAN_GYRO_X) || (chan == SENSOR_CHAN_GYRO_Y) ||
	    (chan == SENSOR_CHAN_GYRO_Z) || (chan == SENSOR_CHAN_DIE_TEMP)) {
#ifdef CONFIG_ICM42670_TRIGGER
		status = icm42670_fetch_from_fifo(dev);
#else
		status = icm42670_fetch_from_registers(dev, chan);
#endif
	}

	icm42670_unlock(dev);
	return status;
}

static int icm42670_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct icm42670_data *drv_data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	icm42670_lock(dev);

	if ((enum sensor_channel_tdk_apex)chan == SENSOR_CHAN_APEX_MOTION) {
		if (attr == SENSOR_ATTR_CONFIGURATION) {
#ifdef CONFIG_TDK_APEX
			if (val->val1 == TDK_APEX_PEDOMETER) {
				icm42670_apex_enable(&drv_data->driver);
				icm42670_apex_enable_pedometer(dev, &drv_data->driver);
			} else if (val->val1 == TDK_APEX_TILT) {
				icm42670_apex_enable(&drv_data->driver);
				icm42670_apex_enable_tilt(&drv_data->driver);
			} else if (val->val1 == TDK_APEX_SMD) {
				icm42670_apex_enable(&drv_data->driver);
				icm42670_apex_enable_smd(&drv_data->driver);
			} else if (val->val1 == TDK_APEX_WOM) {
				icm42670_apex_enable_wom(&drv_data->driver);
			} else {
				LOG_ERR("Not supported ATTR value");
			}
#endif
		} else {
			LOG_ERR("Not supported ATTR");
			return -EINVAL;
		}
	} else if ((chan == SENSOR_CHAN_ACCEL_XYZ) || (chan == SENSOR_CHAN_ACCEL_X) ||
		   (chan == SENSOR_CHAN_ACCEL_Y) || (chan == SENSOR_CHAN_ACCEL_Z)) {
		icm42670_accel_config(drv_data, attr, val);

	} else if ((chan == SENSOR_CHAN_GYRO_XYZ) || (chan == SENSOR_CHAN_GYRO_X) ||
		   (chan == SENSOR_CHAN_GYRO_Y) || (chan == SENSOR_CHAN_GYRO_Z)) {
		icm42670_gyro_config(drv_data, attr, val);

	} else {
		LOG_ERR("Unsupported channel");
		(void)drv_data;
		return -EINVAL;
	}

	icm42670_unlock(dev);

	return 0;
}

static int icm42670_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm42670_data *data = dev->data;
	const struct icm42670_config *cfg = dev->config;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	icm42670_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			val->val1 = data->accel_hz;
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			val->val1 = data->accel_fs;
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;

	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			val->val1 = data->gyro_hz;
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			val->val1 = data->gyro_fs;
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;

	case SENSOR_CHAN_APEX_MOTION:
		if (attr == SENSOR_ATTR_CONFIGURATION) {
			val->val1 = cfg->apex;
		}
		break;

	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	icm42670_unlock(dev);

	return res;
}

static inline int icm42670_bus_check(const struct device *dev)
{
	const struct icm42670_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static int icm42670_init(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	int res = 0;

	if (icm42670_bus_check(dev) < 0) {
		LOG_ERR("bus check failed");
		return -ENODEV;
	}

	data->accel_x = 0;
	data->accel_y = 0;
	data->accel_z = 0;
	data->gyro_x = 0;
	data->gyro_y = 0;
	data->gyro_z = 0;
	data->temp = 0;

	if (icm42670_sensor_init(dev)) {
		LOG_ERR("could not initialize sensor");
		return -EIO;
	}

#ifdef CONFIG_ICM42670_TRIGGER
	res |= icm42670_trigger_enable_interrupt(dev);
	res |= icm42670_trigger_init(dev);
	if (res < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return res;
	}
#endif

	res |= icm42670_turn_on_sensor(dev);

	return res;
}

#ifndef CONFIG_ICM42670_TRIGGER

void icm42670_lock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

void icm42670_unlock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

#endif

static DEVICE_API(sensor, icm42670_driver_api) = {
#ifdef CONFIG_ICM42670_TRIGGER
	.trigger_set = icm42670_trigger_set,
#endif
	.sample_fetch = icm42670_sample_fetch,
	.channel_get = icm42670_channel_get,
	.attr_set = icm42670_attr_set,
	.attr_get = icm42670_attr_get,
};

/* device defaults to spi mode 0/3 support */
#define ICM42670_SPI_CFG (SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA)

/* Initializes a common struct icm42670_config */
#define ICM42670_CONFIG_COMMON(inst)                                                               \
	IF_ENABLED(CONFIG_ICM42670_TRIGGER,	\
				(.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))     \
				    .accel_fs = DT_INST_ENUM_IDX(inst, accel_fs),                  \
				    .accel_hz = DT_INST_ENUM_IDX(inst, accel_hz),                  \
				    .accel_avg = DT_INST_ENUM_IDX(inst, accel_avg),                \
				    .accel_filt_bw = DT_INST_ENUM_IDX(inst, accel_filt_bw_hz),     \
				    .gyro_fs = DT_INST_ENUM_IDX(inst, gyro_fs),                    \
				    .gyro_hz = DT_INST_ENUM_IDX(inst, gyro_hz),                    \
				    .gyro_filt_bw = DT_INST_ENUM_IDX(inst, gyro_filt_bw_hz),       \
				    .accel_pwr_mode = DT_INST_ENUM_IDX(inst, power_mode),          \
				    .apex = DT_INST_ENUM_IDX(inst, apex),

/* Initializes the bus members for an instance on a SPI bus. */
#define ICM42670_CONFIG_SPI(inst)                                                                  \
	{.bus.spi = SPI_DT_SPEC_INST_GET(inst, ICM42670_SPI_CFG, 0),                               \
	 .bus_io = &icm42670_bus_io_spi,                                                           \
	 ICM42670_CONFIG_COMMON(inst)}

/* Initializes the bus members for an instance on an I2C bus. */
#define ICM42670_CONFIG_I2C(inst)                                                                  \
	{.bus.i2c = I2C_DT_SPEC_INST_GET(inst),                                                    \
	 .bus_io = &icm42670_bus_io_i2c,                                                           \
	 ICM42670_CONFIG_COMMON(inst)}

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define ICM42670_DEFINE(inst, name, whoami)                                                        \
	static struct icm42670_data icm42670_data_##inst = {                                       \
		.imu_name = name,                                                                  \
		.imu_whoami = whoami,                                                              \
	};                                                                                         \
	static const struct icm42670_config icm42670_config_##inst = \
			COND_CODE_1(DT_INST_ON_BUS(inst, spi),	\
			(ICM42670_CONFIG_SPI(inst)),	\
			(ICM42670_CONFIG_I2C(inst)));             \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm42670_init, NULL, &icm42670_data_##inst,             \
				     &icm42670_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &icm42670_driver_api);

#define DT_DRV_COMPAT invensense_icm42670p
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(ICM42670_DEFINE, INV_ICM42670P_STRING_ID, INV_ICM42670P_WHOAMI);
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT invensense_icm42670s
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(ICM42670_DEFINE, INV_ICM42670S_STRING_ID, INV_ICM42670S_WHOAMI);
#endif
