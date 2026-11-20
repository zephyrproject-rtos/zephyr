/*
 * Copyright (c) 2026 Meta Platforms, Inc. and affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "icm45686.h"

struct gyro_range {
	uint8_t fs;
	uint32_t mdps;
};

static const struct gyro_range gyro_ranges[] = {
	{ICM45686_DT_GYRO_FS_4000, 4000000}, {ICM45686_DT_GYRO_FS_2000, 2000000},
	{ICM45686_DT_GYRO_FS_1000, 1000000}, {ICM45686_DT_GYRO_FS_500, 500000},
	{ICM45686_DT_GYRO_FS_250, 250000},   {ICM45686_DT_GYRO_FS_125, 125000},
	{ICM45686_DT_GYRO_FS_62_5, 62500},   {ICM45686_DT_GYRO_FS_31_25, 31250},
	{ICM45686_DT_GYRO_FS_15_625, 15625},
};

/* in counts at the given range, in micro rad/s, with Zephyr's value of pi */
static double expected_urads(const struct gyro_range *r, int32_t in, bool high_res)
{
	const double counts = high_res ? 524288.0 : 32768.0;

	return (double)in * (r->mdps / 1000.0) / counts * (double)SENSOR_PI / 180.0;
}

static void check_gyro(bool high_res)
{
	const int32_t full = high_res ? 524287 : 32767;
	const int32_t inputs[] = {full, -full - 1, 1, -1, 12345, -20000};

	ARRAY_FOR_EACH(gyro_ranges, i) {
		ARRAY_FOR_EACH(inputs, j) {
			int32_t rads, urads;

			icm45686_gyro_rads(gyro_ranges[i].fs, inputs[j], high_res, &rads, &urads);

			double got = (double)rads * 1000000.0 + (double)urads;
			double want = expected_urads(&gyro_ranges[i], inputs[j], high_res);

			zassert_within(got, want, 1.0,
				       "%u mdps, %s, in %d: got %lld urad/s, want %lld",
				       gyro_ranges[i].mdps, high_res ? "20-bit" : "16-bit",
				       inputs[j], (long long)got, (long long)want);
		}
	}
}

ZTEST(icm45686_convert, test_gyro_rads_16bit)
{
	check_gyro(false);
}

ZTEST(icm45686_convert, test_gyro_rads_20bit)
{
	check_gyro(true);
}

ZTEST(icm45686_convert, test_gyro_rads_full_scale)
{
	int32_t rads, urads;

	/* A full-scale count at 2000 dps is 2000 dps */
	icm45686_gyro_rads(ICM45686_DT_GYRO_FS_2000, 32768, false, &rads, &urads);
	zassert_equal(rads, 34);
	zassert_within(urads, 906577, 1);

	/* Negative values keep the sign in both parts */
	icm45686_gyro_rads(ICM45686_DT_GYRO_FS_2000, -32768, false, &rads, &urads);
	zassert_equal(rads, -34);
	zassert_within(urads, -906577, 1);
}

ZTEST(icm45686_convert, test_accel_reg_to_fs)
{
	static const struct {
		uint8_t fs;
		int64_t g;
	} ranges[] = {
		{ICM45686_DT_ACCEL_FS_32, 32}, {ICM45686_DT_ACCEL_FS_16, 16},
		{ICM45686_DT_ACCEL_FS_8, 8},   {ICM45686_DT_ACCEL_FS_4, 4},
		{ICM45686_DT_ACCEL_FS_2, 2},
	};

	ARRAY_FOR_EACH(ranges, i) {
		struct sensor_value val;
		int64_t want_ums = ranges[i].g * SENSOR_G;

		icm45686_accel_reg_to_fs(ranges[i].fs, &val);

		zassert_equal(val.val1, want_ums / 1000000, "%lld g", ranges[i].g);
		zassert_equal(val.val2, want_ums % 1000000, "%lld g", ranges[i].g);
	}
}

ZTEST(icm45686_convert, test_gyro_reg_to_fs)
{
	ARRAY_FOR_EACH(gyro_ranges, i) {
		struct sensor_value val;

		icm45686_gyro_reg_to_fs(gyro_ranges[i].fs, &val);

		double got = (double)val.val1 * 1000000.0 + (double)val.val2;
		double want = expected_urads(&gyro_ranges[i], 32768, false);

		zassert_within(got, want, 1.0, "%u mdps: got %lld urad/s, want %lld",
			       gyro_ranges[i].mdps, (long long)got, (long long)want);
	}
}

ZTEST_SUITE(icm45686_convert, NULL, NULL, NULL, NULL, NULL);
