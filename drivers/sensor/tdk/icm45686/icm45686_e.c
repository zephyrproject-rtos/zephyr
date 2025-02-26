/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm45686.h"

#include <zephyr/drivers/sensor/icm45686.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>

static uint32_t convert_gyr_fs_to_bitfield(uint32_t val, uint16_t *fs)
{
	uint32_t bitfield = 0;

	return bitfield;
}

static int icm45686_set_gyro_odr(struct icm45686_data *drv_data, const struct sensor_value *val)
{
	return 0;
}

static int icm45686_set_gyro_fs(struct icm45686_data *drv_data, const struct sensor_value *val)
{
	return 0;
}

int icm45686_gyro_config(struct icm45686_data *drv_data, enum sensor_attribute attr,
			 const struct sensor_value *val)
{
	return 0;
}

void icm45686_convert_gyro(struct sensor_value *val, int16_t raw_val, uint16_t fs)
{
	return;
}

int icm45686_sample_fetch_gyro(const struct device *dev)
{
	return 0;
}

uint16_t convert_bitfield_to_gyr_fs(uint8_t bitfield)
{
	uint16_t gyr_fs = 0;

	return gyr_fs;
}
