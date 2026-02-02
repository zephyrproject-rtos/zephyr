/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/drivers/sensor.h>

#include "adxl345_features.h"

void adxl345_accel_convert(struct sensor_value *val, int16_t sample, uint8_t selected_range)
{
	const int32_t sensitivity[] = {
		[ADXL345_RANGE_2G] = INT32_C(SENSOR_G / 256),
		[ADXL345_RANGE_4G] = INT32_C(SENSOR_G / 128),
		[ADXL345_RANGE_8G] = INT32_C(SENSOR_G / 64),
		[ADXL345_RANGE_16G] = INT32_C(SENSOR_G / 32),
	};

	if (sample & BIT(9)) {
		sample |= ADXL345_COMPLEMENT;
	}

	val->val1 = (sample * sensitivity[selected_range]) / 1000000;
	val->val2 = (sample * sensitivity[selected_range]) % 1000000;
}
