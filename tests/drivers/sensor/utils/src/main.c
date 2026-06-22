/*
 * Copyright (c) 2026 Ale Alfaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/sensor.h>

/*
 * Tests and usage reference for the sensor fixed-point helpers that convert sensor
 * readings to and from SI units.
 *
 * See the "Fixed-point values" section in
 * doc/hardware/peripherals/sensor/read_and_decode.rst for the Q31 background.
 */

#define ACCEL_SHIFT    10
#define GYRO_SHIFT     10
#define TEMP_SHIFT     10
/*
 * The q31 scaling at a given shift that can be multiplied to convert a value to fixed point, i.e.
 * 2^(31 - shift). Expected values are written as a multiple of this to make it clear what this
 * values are really in terms of SI units
 */
#define Q31_ONE(shift) ((q31_t)BIT(31 - (shift)))

/* Q31 conversion constants */
/* Accel sensor: fs=8g 8/32768 = 1/4096 g per LSB. */
#define ACCEL_SCALING   Q31_ONE(ACCEL_SHIFT)
#define ACCEL_FS_RANGE  8
#define ACCEL_LSB_PER_G (4096)
#define GYRO_SCALING    Q31_ONE(GYRO_SHIFT)
#define GYRO_RANGE_DPS  1000
#define TEMP_SCALING    Q31_ONE(TEMP_SHIFT)

/* A signed 16-bit sample spans its full-scale range over +/- 2^15 counts. */
#define SAMPLE_WIDTH_BITS 16
#define SAMPLE_FULL_SCALE (1 << (SAMPLE_WIDTH_BITS - 1)) /* signed: negative raw divides right */

/* Temperature sensor: 1/512 deg C per LSB, offset of 23.000 C. */
#define TEMP_LSB_PER_C                512
#define TEMP_OFFSET_MICRO_DEG_CELSIUS (23000000LL)

#define MICRO_PER_UNIT 1000000LL
#define MILLI_PER_UNIT 1000LL
#define CENTI_PER_UNIT 100LL

ZTEST_SUITE(sensor_fp, NULL, NULL, NULL, NULL, NULL);

/*
 * SI units -> q31  tests
 */
struct si_units_to_q31_case {
	int64_t si_val;
	q31_t expected;
};

/* Conversion to q31 from micro units */
ZTEST_P(sensor_fp, test_accel_from_micro)
{
	const struct si_units_to_q31_case *c = ZTEST_GET_PARAM_PTR(struct si_units_to_q31_case);
	q31_t q = sensor_q31_from_micro(c->si_val, ACCEL_SHIFT);

	zexpect_equal(q, c->expected, "%lld si_val-g @ shift %d: got %d, expected %d",
		      (long long)c->si_val, ACCEL_SHIFT, q, c->expected);
}

static const struct si_units_to_q31_case accel_micro_normal[] = {
	{.si_val = 1 * MICRO_PER_UNIT, .expected = ACCEL_SCALING},     /* +1.00 g */
	{.si_val = MICRO_PER_UNIT / 2, .expected = ACCEL_SCALING / 2}, /* +0.50 g */
	{.si_val = -1 * MICRO_PER_UNIT, .expected = -ACCEL_SCALING},   /* -1.00 g */
};
ZTEST_DEFINE_PARAM_VALUES_ARRAY(accel_micro_normal_vals, accel_micro_normal);
ZTEST_INSTANTIATE_TEST_SUITE_P(accel_micro_normal, sensor_fp, test_accel_from_micro,
			       accel_micro_normal_vals);

/* Conversion to q31 from milli units */
ZTEST_P(sensor_fp, test_gyro_from_milli)
{
	const struct si_units_to_q31_case *c = ZTEST_GET_PARAM_PTR(struct si_units_to_q31_case);
	q31_t q = sensor_q31_from_milli(c->si_val, GYRO_SHIFT);

	zexpect_equal(q, c->expected, "%lld milli-dps @ shift %d: got %d, expected %d",
		      (long long)c->si_val, GYRO_SHIFT, q, c->expected);
}

static const struct si_units_to_q31_case gyro_milli_normal[] = {
	{.si_val = 250 * MILLI_PER_UNIT, .expected = 250 * GYRO_SCALING},   /* +250 dps */
	{.si_val = MILLI_PER_UNIT / 2, .expected = GYRO_SCALING / 2},       /* +0.5 dps */
	{.si_val = -250 * MILLI_PER_UNIT, .expected = -250 * GYRO_SCALING}, /* -250 dps */
};
ZTEST_DEFINE_PARAM_VALUES_ARRAY(gyro_milli_normal_vals, gyro_milli_normal);
ZTEST_INSTANTIATE_TEST_SUITE_P(gyro_milli, sensor_fp, test_gyro_from_milli, gyro_milli_normal_vals);

/* Conversion to q31 from centi units */
ZTEST_P(sensor_fp, test_temp_from_centi)
{
	const struct si_units_to_q31_case *c = ZTEST_GET_PARAM_PTR(struct si_units_to_q31_case);
	q31_t q = sensor_q31_from_centi(c->si_val, TEMP_SHIFT);

	zexpect_equal(q, c->expected, "%lld centi-C @ shift %d: got %d, expected %d",
		      (long long)c->si_val, TEMP_SHIFT, q, c->expected);
}

static const struct si_units_to_q31_case temp_centi_normal[] = {
	{.si_val = 23 * CENTI_PER_UNIT, .expected = 23 * TEMP_SCALING},   /*  23.00 C */
	{.si_val = -10 * CENTI_PER_UNIT, .expected = -10 * TEMP_SCALING}, /* -10.00 C */
	{.si_val = 0, .expected = 0},                                     /*   0.00 C */
};
ZTEST_DEFINE_PARAM_VALUES_ARRAY(temp_centi_normal_vals, temp_centi_normal);
ZTEST_INSTANTIATE_TEST_SUITE_P(temp_centi, sensor_fp, test_temp_from_centi, temp_centi_normal_vals);

/*
 * q31 -> SI units tests
 */
struct q31_to_si_units_case {
	q31_t q;
	int64_t expected;
};

/* Conversion to micro units from q31 */
ZTEST_P(sensor_fp, test_accel_to_micro)
{
	const struct q31_to_si_units_case *c = ZTEST_GET_PARAM_PTR(struct q31_to_si_units_case);
	int64_t micro = sensor_q31_to_micro(c->q, ACCEL_SHIFT);

	zexpect_equal(micro, c->expected, "q31 %d @ shift %d: got %lld, expected %lld", c->q,
		      ACCEL_SHIFT, (long long)micro, (long long)c->expected);
}

static const struct q31_to_si_units_case accel_to_micro[] = {
	{.q = ACCEL_SCALING, .expected = MICRO_PER_UNIT},         /* 1,000,000 micro g */
	{.q = ACCEL_SCALING / 2, .expected = MICRO_PER_UNIT / 2}, /* 500,000 micro g */
	{.q = -ACCEL_SCALING, .expected = -1 * MICRO_PER_UNIT},   /* -1,000,000 micro g */
};
ZTEST_DEFINE_PARAM_VALUES_ARRAY(accel_to_micro_vals, accel_to_micro);
ZTEST_INSTANTIATE_TEST_SUITE_P(accel_to_micro, sensor_fp, test_accel_to_micro, accel_to_micro_vals);

/* Conversion to milli units from q31 */
ZTEST_P(sensor_fp, test_gyro_to_milli)
{
	const struct q31_to_si_units_case *c = ZTEST_GET_PARAM_PTR(struct q31_to_si_units_case);
	int64_t milli = sensor_q31_to_milli(c->q, GYRO_SHIFT);

	zexpect_equal(milli, c->expected, "q31 %d @ shift %d: got %lld, expected %lld", c->q,
		      GYRO_SHIFT, (long long)milli, (long long)c->expected);
}

static const struct q31_to_si_units_case gyro_to_milli[] = {
	{.q = GYRO_SCALING, .expected = MILLI_PER_UNIT},         /*  1000 milli-dps */
	{.q = GYRO_SCALING / 2, .expected = MILLI_PER_UNIT / 2}, /* 500 milli-dps */
	{.q = -GYRO_SCALING, .expected = -1 * MILLI_PER_UNIT},   /* -1000 milli-dps */
};
ZTEST_DEFINE_PARAM_VALUES_ARRAY(gyro_to_milli_vals, gyro_to_milli);
ZTEST_INSTANTIATE_TEST_SUITE_P(gyro_to_milli, sensor_fp, test_gyro_to_milli, gyro_to_milli_vals);

/* Conversion to centi units from q31 */
ZTEST_P(sensor_fp, test_temp_to_centi)
{
	const struct q31_to_si_units_case *c = ZTEST_GET_PARAM_PTR(struct q31_to_si_units_case);
	int64_t centi = sensor_q31_to_centi(c->q, TEMP_SHIFT);

	zexpect_equal(centi, c->expected, "q31 %d @ shift %d: got %lld, expected %lld", c->q,
		      TEMP_SHIFT, (long long)centi, (long long)c->expected);
}

static const struct q31_to_si_units_case temp_to_centi[] = {
	{.q = 23 * TEMP_SCALING, .expected = 23 * CENTI_PER_UNIT},   /*  2300 centi C */
	{.q = TEMP_SCALING / 2, .expected = CENTI_PER_UNIT / 2},     /*  50 centi C */
	{.q = -23 * TEMP_SCALING, .expected = -23 * CENTI_PER_UNIT}, /* -2300 centi C */
};
ZTEST_DEFINE_PARAM_VALUES_ARRAY(temp_to_centi_vals, temp_to_centi);
ZTEST_INSTANTIATE_TEST_SUITE_P(temp_to_centi, sensor_fp, test_temp_to_centi, temp_to_centi_vals);

/*
 * value -> q31 -> value : exactly representable readings come back unchanged.
 */
ZTEST(sensor_fp, test_roundtrip)
{
	q31_t q;
	int64_t expected;

	/* accelerometer: +1.5 g in micro-g */
	expected = (3 * MICRO_PER_UNIT) / 2;
	q = sensor_q31_from_micro(expected, ACCEL_SHIFT);
	zexpect_equal(sensor_q31_to_micro(q, ACCEL_SHIFT), expected, "accel 1.5 g round-trip");

	/* temperature: 23.00 C in centi */
	expected = 23 * CENTI_PER_UNIT;
	q = sensor_q31_from_centi(expected, TEMP_SHIFT);
	zexpect_equal(sensor_q31_to_centi(q, TEMP_SHIFT), expected, "temp 23.00 C round-trip");

	/* gyroscope: 250.0 dps in milli */
	expected = 250 * MILLI_PER_UNIT;
	q = sensor_q31_from_milli(expected, GYRO_SHIFT);
	zexpect_equal(sensor_q31_to_milli(q, GYRO_SHIFT), expected, "gyro 250 dps round-trip");
}

/*
 * value -> q31 from a raw register read -- the real driver scenario.
 *
 * A driver turns the raw sample into an SI value using the sensor's full-scale
 * range / sensitivity (and offset), then encodes that as q31.
 */
struct raw_to_q31_case {
	int16_t raw;
	q31_t expected;
};

/* Accelerometer: 16-bit signed sample, +/- 8 g full-scale (so 1 g = 4096 LSB). */
ZTEST_P(sensor_fp, test_accel_from_raw)
{
	const struct raw_to_q31_case *c = ZTEST_GET_PARAM_PTR(struct raw_to_q31_case);
	/* raw counts -> micro-g: full-scale ACCEL_FS_RANGE spans SAMPLE_FULL_SCALE counts. */
	int64_t micro_g = ((int64_t)c->raw * ACCEL_FS_RANGE * MICRO_PER_UNIT) / SAMPLE_FULL_SCALE;
	q31_t q = sensor_q31_from_micro(micro_g, ACCEL_SHIFT);

	zexpect_equal(q, c->expected, "raw %d: got %d, expected %d", c->raw, q, c->expected);
}

static const struct raw_to_q31_case accel_from_raw[] = {
	{.raw = ACCEL_LSB_PER_G, .expected = ACCEL_SCALING},         /* +1.0 g */
	{.raw = ACCEL_LSB_PER_G / 2, .expected = ACCEL_SCALING / 2}, /* +0.5 g */
	{.raw = 2 * ACCEL_LSB_PER_G, .expected = 2 * ACCEL_SCALING}, /* +2.0 g */
	{.raw = -ACCEL_LSB_PER_G, .expected = -ACCEL_SCALING},       /* -1.0 g */
};
ZTEST_DEFINE_PARAM_VALUES_ARRAY(accel_raw_vals, accel_from_raw);
ZTEST_INSTANTIATE_TEST_SUITE_P(accel_raw, sensor_fp, test_accel_from_raw, accel_raw_vals);

/* Temperature: 16-bit sample, 512 LSB/deg C, reading 23 C at raw 0. */
ZTEST_P(sensor_fp, test_temp_from_raw)
{
	const struct raw_to_q31_case *c = ZTEST_GET_PARAM_PTR(struct raw_to_q31_case);
	/* raw counts -> micro-C: apply sensitivity (LSB/C) and the 23 C offset. */
	int64_t micro_c =
		TEMP_OFFSET_MICRO_DEG_CELSIUS + ((int64_t)c->raw * MICRO_PER_UNIT) / TEMP_LSB_PER_C;
	q31_t q = sensor_q31_from_micro(micro_c, TEMP_SHIFT);

	zexpect_equal(q, c->expected, "raw %d: got q31=%d (%d C), expected q31=%d (%d C)", c->raw,
		      q, (q / TEMP_SCALING), c->expected, (c->expected / TEMP_SCALING));
}

/* Gyroscope: 16-bit signed sample, +/- 1000 dps full-scale. */
ZTEST_P(sensor_fp, test_gyro_from_raw)
{
	const struct raw_to_q31_case *c = ZTEST_GET_PARAM_PTR(struct raw_to_q31_case);
	/* raw counts -> milli-dps: full-scale GYRO_RANGE_DPS spans SAMPLE_FULL_SCALE counts. */
	int64_t milli_dps = ((int64_t)c->raw * GYRO_RANGE_DPS * MILLI_PER_UNIT) / SAMPLE_FULL_SCALE;
	q31_t q = sensor_q31_from_milli(milli_dps, GYRO_SHIFT);

	zexpect_equal(q, c->expected, "raw %d: got %d, expected %d", c->raw, q, c->expected);
}

static const struct raw_to_q31_case gyro_from_raw[] = {
	{.raw = SAMPLE_FULL_SCALE / 2, .expected = 500 * GYRO_SCALING},     /* +500 dps */
	{.raw = SAMPLE_FULL_SCALE / 4, .expected = 250 * GYRO_SCALING},     /* +250 dps */
	{.raw = SAMPLE_FULL_SCALE / 8, .expected = 125 * GYRO_SCALING},     /* +125 dps */
	{.raw = -(SAMPLE_FULL_SCALE / 2), .expected = -500 * GYRO_SCALING}, /* -500 dps */
};
ZTEST_DEFINE_PARAM_VALUES_ARRAY(gyro_raw_vals, gyro_from_raw);
ZTEST_INSTANTIATE_TEST_SUITE_P(gyro_raw, sensor_fp, test_gyro_from_raw, gyro_raw_vals);

static const struct raw_to_q31_case temp_from_raw[] = {
	{.raw = 0, .expected = 23 * TEMP_SCALING},                   /* 23.0 C */
	{.raw = TEMP_LSB_PER_C, .expected = 24 * TEMP_SCALING},      /* 24.0 C */
	{.raw = -TEMP_LSB_PER_C, .expected = 22 * TEMP_SCALING},     /* 22.0 C */
	{.raw = 10 * TEMP_LSB_PER_C, .expected = 33 * TEMP_SCALING}, /* 33.0 C */
};
ZTEST_DEFINE_PARAM_VALUES_ARRAY(temp_raw_vals, temp_from_raw);
ZTEST_INSTANTIATE_TEST_SUITE_P(temp_raw, sensor_fp, test_temp_from_raw, temp_raw_vals);
