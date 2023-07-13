/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AKM09918C_AKM09918C_H_
#define ZEPHYR_DRIVERS_SENSOR_AKM09918C_AKM09918C_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

#include "akm09918c_reg.h"

/* Time it takes to get a measurement in single-measure mode */
#define AKM09918C_MEASURE_TIME_US 9000

/* Conversion values */
#define AKM09918C_MICRO_GAUSS_PER_BIT INT64_C(1500)

/* Maximum and minimum raw register values for magnetometer data per datasheet */
#define AKM09918C_MAGN_MAX_DATA_REG (32752)
#define AKM09918C_MAGN_MIN_DATA_REG (-32752)

/* Maximum and minimum magnetometer values in microgauss. +/-32752 is the maximum range of the
 * data registers (slightly less than the range of int16). This works out to +/- 49,128,000 uGs
 */
#define AKM09918C_MAGN_MAX_MICRO_GAUSS (AKM09918C_MAGN_MAX_DATA_REG * AKM09918C_MICRO_GAUSS_PER_BIT)
#define AKM09918C_MAGN_MIN_MICRO_GAUSS (AKM09918C_MAGN_MIN_DATA_REG * AKM09918C_MICRO_GAUSS_PER_BIT)

struct akm09918c_data {
	int16_t x_sample;
	int16_t y_sample;
	int16_t z_sample;
	uint8_t mode;
};

struct akm09918c_config {
	struct i2c_dt_spec i2c;
};

static inline uint8_t akm09918c_hz_to_reg(const struct sensor_value *val)
{
	if (val->val1 >= 100) {
		return AKM09918C_CNTL2_CONTINUOUS_4;
	} else if (val->val1 >= 50) {
		return AKM09918C_CNTL2_CONTINUOUS_3;
	} else if (val->val1 >= 20) {
		return AKM09918C_CNTL2_CONTINUOUS_2;
	} else if (val->val1 > 0) {
		return AKM09918C_CNTL2_CONTINUOUS_1;
	} else {
		return AKM09918C_CNTL2_PWR_DOWN;
	}
}

static inline void akm09918c_reg_to_hz(uint8_t reg, struct sensor_value *val)
{
	val->val1 = 0;
	val->val2 = 0;
	switch (reg) {
	case AKM09918C_CNTL2_CONTINUOUS_1:
		val->val1 = 10;
		break;
	case AKM09918C_CNTL2_CONTINUOUS_2:
		val->val1 = 20;
		break;
	case AKM09918C_CNTL2_CONTINUOUS_3:
		val->val1 = 50;
		break;
	case AKM09918C_CNTL2_CONTINUOUS_4:
		val->val1 = 100;
		break;
	}
}

#endif /* ZEPHYR_DRIVERS_SENSOR_AKM09918C_AKM09918C_H_ */
