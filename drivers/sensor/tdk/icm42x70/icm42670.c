/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm42670.h"

#include <zephyr/drivers/sensor/icm42x70.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42670, CONFIG_SENSOR_LOG_LEVEL);

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

static int icm42670_set_gyro_odr(struct icm42x70_data *drv_data, const struct sensor_value *val)
{
	if (val->val1 <= 1600 && val->val1 >= 12) {
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

static int icm42670_set_gyro_fs(struct icm42x70_data *drv_data, const struct sensor_value *val)
{
	int32_t val_dps = sensor_rad_to_degrees(val);

	if (val_dps > 2000 || val_dps < 250) {
		LOG_ERR("Incorrect fullscale value");
		return -EINVAL;
	}
	inv_imu_set_gyro_fsr(&drv_data->driver,
			     convert_gyr_fs_to_bitfield(val_dps, &drv_data->gyro_fs));
	LOG_DBG("Set gyro fullscale to: %d dps", drv_data->gyro_fs);
	return 0;
}

int icm42670_gyro_config(struct icm42x70_data *drv_data, enum sensor_attribute attr,
			 const struct sensor_value *val)
{
	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		icm42670_set_gyro_odr(drv_data, val);

	} else if (attr == SENSOR_ATTR_FULL_SCALE) {
		icm42670_set_gyro_fs(drv_data, val);

	} else if ((enum sensor_attribute_icm42x70)attr == SENSOR_ATTR_BW_FILTER_LPF) {
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

void icm42670_convert_gyro(struct sensor_value *val, int16_t raw_val, uint16_t fs)
{
	int64_t conv_val;

	/* 16 bit gyroscope. 2^15 bits represent the range in degrees/s */
	/* see datasheet section 3.1 for details */
	conv_val = ((int64_t)raw_val * fs * SENSOR_PI) / (INT16_MAX * 180U);

	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

int icm42670_sample_fetch_gyro(const struct device *dev)
{
	struct icm42x70_data *data = dev->data;
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

uint16_t convert_bitfield_to_gyr_fs(uint8_t bitfield)
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
