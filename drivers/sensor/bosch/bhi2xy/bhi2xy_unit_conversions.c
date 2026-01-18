/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bhi2xy_unit_conversions.h"

/* simple division conversions, when range is fixed and there is no range */
#define BHI2XY_MAG_BMM150_DIVISOR  1600.0f  /* LSB/Gauss, for BMM150 magnetometer */
#define BHI2XY_ORI_DIVISOR         91.0222f /* LSB/deg (32768 / 360) */
#define BHI2XY_QUAT_DIVISOR        16384.0f
#define BHI2XY_PRES_BMP390_DIVISOR 100.0 /* LSB/kPa, for BMP390 pressure sensor */

typedef struct {
	uint16_t range;
	float divisor;
} bhi2xy_divisor;

/* range in g, divisor in LSB/ms2 */
#define G_TO_MS2_SCALE 9.80665f
static const bhi2xy_divisor accel_divisors[] = {
	{2, 16384.0f * G_TO_MS2_SCALE},
	{4, 8192.0f * G_TO_MS2_SCALE},
	{8, 4096.0f * G_TO_MS2_SCALE},
	{16, 2048.0f * G_TO_MS2_SCALE},
};

/* range in dps, divisor in LSB/rads */
#define DEG_TO_RAD_SCALE (3.14159265359f / 180.0f)
static const bhi2xy_divisor gyro_divisors[] = {
	{125, 262.4f * DEG_TO_RAD_SCALE}, {250, 131.2f * DEG_TO_RAD_SCALE},
	{500, 65.6f * DEG_TO_RAD_SCALE},  {1000, 32.8f * DEG_TO_RAD_SCALE},
	{2000, 16.4f * DEG_TO_RAD_SCALE},
};

static float bhi2xy_get_divisor(uint16_t range, const bhi2xy_divisor *divisors, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (divisors[i].range == range) {
			return divisors[i].divisor;
		}
	}
	/* if range not found, return the divisor for the lowest range */
	return divisors[0].divisor;
}

void bhi2xy_accel_to_ms2(struct sensor_value *val, int16_t raw_val, uint16_t range)
{
	float divisor = bhi2xy_get_divisor(range, accel_divisors,
					   sizeof(accel_divisors) / sizeof(accel_divisors[0]));
	sensor_value_from_float(val, raw_val / divisor);
}

void bhi2xy_gyro_to_rads(struct sensor_value *val, int16_t raw_val, uint16_t range)
{
	float divisor = bhi2xy_get_divisor(range, gyro_divisors,
					   sizeof(gyro_divisors) / sizeof(gyro_divisors[0]));
	sensor_value_from_float(val, raw_val / divisor);
}

void bhi2xy_mag_to_gauss(struct sensor_value *val, int16_t raw_val)
{
	sensor_value_from_float(val, raw_val / BHI2XY_MAG_BMM150_DIVISOR);
}

void bhi2xy_pres_to_kpa(struct sensor_value *val, uint32_t raw_val)
{
	sensor_value_from_float(val, raw_val / BHI2XY_PRES_BMP390_DIVISOR);
}

void bhi2xy_rv_grv_to_quat(struct sensor_value *val, int16_t raw_val)
{
	sensor_value_from_float(val, raw_val / BHI2XY_QUAT_DIVISOR);
}

void bhi2xy_ori_to_deg(struct sensor_value *val, int16_t raw_val)
{
	sensor_value_from_float(val, raw_val / BHI2XY_ORI_DIVISOR);
}
