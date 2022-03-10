/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup driver_sensor_subsys_tests sensor_subsys
 * @ingroup all_tests
 * @{
 * @}
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/emul/bmi160.h>
#include <zephyr/math/util.h>
#include <zephyr/ztest.h>

#include <bmi160.h>

struct sensor_accel_fixture {
	const struct device *accel_0;
	const struct device *accel_1;
	const struct emul *accel_emul_0;
	const struct emul *accel_emul_1;
};

void test_sensor_accel_basic(const struct device *dev)
{
	SENSOR_DATA(struct sensor_three_axis_data, data, 1);
	struct sensor_scale_metadata scale_metadata[2];

	zassert_ok(sensor_read_data(dev, SENSOR_TYPE_ACCELEROMETER, data), NULL);
	zassert_equal(1, data->header.reading_count, NULL);
	/* Test raw values */
	zassert_equal(0x0001, data->readings[0].x, NULL);
	zassert_equal(0x0689, data->readings[0].y, NULL);
	zassert_equal(0x0d11, data->readings[0].z, NULL);

	/* Get the scale metadata for both sensor types */
	zassert_ok(sensor_get_scale(dev, SENSOR_TYPE_ACCELEROMETER, &scale_metadata[0]), NULL);
	zassert_ok(sensor_get_scale(dev, SENSOR_TYPE_GYROSCOPE, &scale_metadata[1]), NULL);
	zassert_equal(SENSOR_RANGE_UNITS_ACCEL_G, scale_metadata[0].range_units, NULL);
	zassert_equal(SENSOR_RANGE_UNITS_ANGLE_DEGREES, scale_metadata[1].range_units, NULL);

	/* Test converted values in SI units */
	sensor_sample_to_three_axis_data(&scale_metadata[0], data, 0);
	zassert_within(FLOAT_TO_FP(0.0f), data->readings[0].x, FLOAT_TO_FP(0.01f),
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf,
		       PRIf_ARG(FLOAT_TO_FP(0.0f)), PRIf_ARG(FLOAT_TO_FP(0.01f)),
		       PRIf_ARG(data->readings[0].x));
	zassert_within(FLOAT_TO_FP(0.1f), data->readings[0].y, FLOAT_TO_FP(0.01f),
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf,
		       PRIf_ARG(FLOAT_TO_FP(0.1f)), PRIf_ARG(FLOAT_TO_FP(0.01f)),
		       PRIf_ARG(data->readings[0].y));
	zassert_within(FLOAT_TO_FP(0.2f), data->readings[0].z, FLOAT_TO_FP(0.01f),
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf,
		       PRIf_ARG(FLOAT_TO_FP(0.2f)), PRIf_ARG(FLOAT_TO_FP(0.01f)),
		       PRIf_ARG(data->readings[0].z));

	zassert_ok(sensor_read_data(dev, SENSOR_TYPE_GYROSCOPE, data), NULL);
	zassert_equal(0x0b01, data->readings[0].x, NULL);
	zassert_equal(0x0eac, data->readings[0].y, NULL);
	zassert_equal(0x1257, data->readings[0].z, NULL);

	/* Test converted values in SI units */
	sensor_sample_to_three_axis_data(&scale_metadata[1], data, 0);
	zassert_within(FLOAT_TO_FP(171.9f), data->readings[0].x, FLOAT_TO_FP(0.1f),
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf,
		       PRIf_ARG(FLOAT_TO_FP(171.9f)), PRIf_ARG(FLOAT_TO_FP(0.1f)),
		       PRIf_ARG(data->readings[0].x));
	zassert_within(FLOAT_TO_FP(229.2f), data->readings[0].y, FLOAT_TO_FP(0.1f),
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf,
		       PRIf_ARG(FLOAT_TO_FP(229.2f)), PRIf_ARG(FLOAT_TO_FP(0.1f)),
		       PRIf_ARG(data->readings[0].y));
	zassert_within(FLOAT_TO_FP(286.6f), data->readings[0].z, FLOAT_TO_FP(0.1f),
		       "Expected %" PRIf " to be within %" PRIf " of %" PRIf,
		       PRIf_ARG(FLOAT_TO_FP(286.6f)), PRIf_ARG(FLOAT_TO_FP(0.1f)),
		       PRIf_ARG(data->readings[0].z));
}

ZTEST_F(sensor_accel, test_accel_0_read_data) { test_sensor_accel_basic(fixture->accel_0); }

ZTEST_F(sensor_accel, test_accel_1_read_data) { test_sensor_accel_basic(fixture->accel_1); }

ZTEST_F(sensor_accel, test_set_bias)
{
	int8_t bias[3];

	zassert_ok(sensor_set_bias(fixture->accel_1, SENSOR_TYPE_ACCELEROMETER, INT16_C(20),
				   FLOAT_TO_FP(4.0f), FLOAT_TO_FP(-8.0f), FLOAT_TO_FP(12.0f), true),
		   NULL);
	zassert_ok(bmi160_emul_get_bias(fixture->accel_emul_1, SENSOR_TYPE_ACCELEROMETER, &bias[0],
					&bias[1], &bias[2]),
		   NULL);
	zassert_equal(1, bias[0], NULL);
	zassert_equal(-3, bias[1], NULL);
	zassert_equal(3, bias[2], NULL);
}

ZTEST_F(sensor_accel, test_get_bias)
{
	int16_t temperature;
	fp_t bias[3];

	zassert_ok(
		bmi160_emul_set_bias(fixture->accel_emul_1, SENSOR_TYPE_ACCELEROMETER, 5, -5, 17),
		NULL);
	zassert_ok(sensor_get_bias(fixture->accel_1, SENSOR_TYPE_ACCELEROMETER, &temperature,
				   &bias[0], &bias[1], &bias[2]),
		   NULL);
	zassert_equal(INT16_MIN, temperature, NULL);
	zassert_within(FLOAT_TO_FP(19.5f), bias[0], FLOAT_TO_FP(0.1f), NULL);
	zassert_within(FLOAT_TO_FP(-19.5f), bias[1], FLOAT_TO_FP(0.1f), NULL);
	zassert_within(FLOAT_TO_FP(66.3f), bias[2], FLOAT_TO_FP(0.1f), NULL);
}

static bool sensor_sample_rate_info_contains(const struct sensor_sample_rate_info *info,
					     uint8_t size, uint32_t type, uint32_t rate)
{
	for (uint8_t i = 0; i < size; ++i) {
		if (info[i].sensor_type == type && info[i].sample_rate_mhz == rate) {
			return true;
		}
	}
	return false;
}

ZTEST_F(sensor_accel, test_get_sample_rates)
{
	const struct sensor_sample_rate_info *sample_rate_info;
	uint8_t count;


	zassert_ok(sensor_get_sample_rate_available(fixture->accel_1, &sample_rate_info, &count),
		   NULL);
	zassert_equal(36, count, NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(781)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(1563)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(3125)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(6250)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(12500)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(25000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(50000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(100000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(200000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(400000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(800000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_ACCELEROMETER, UINT32_C(1600000)),
		     NULL);

	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(781)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(1563)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(3125)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(6250)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(12500)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(25000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(50000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(100000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(200000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(400000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(800000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(1600000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GYROSCOPE, UINT32_C(3200000)),
		     NULL);

	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD, UINT32_C(781)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(1563)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(3125)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(6250)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(12500)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(25000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(50000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(100000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(200000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(400000)),
		     NULL);
	zassert_true(sensor_sample_rate_info_contains(sample_rate_info, count,
						      SENSOR_TYPE_GEOMAGNETIC_FIELD,
						      UINT32_C(800000)),
		     NULL);
}

ZTEST_F(sensor_accel, test_set_watermark_0)
{
	uint8_t int_status;
	uint8_t watermark_reg_val;

	/* Set all the bits */
	zassert_ok(bmi160_emul_set_int_status_reg(fixture->accel_emul_1, 1, UINT8_MAX), NULL);
	zassert_ok(bmi160_emul_set_watermark_reg(fixture->accel_emul_1, UINT8_MAX), NULL);

	/* Set the watermark to 0 */
	zassert_ok(sensor_fifo_set_watermark(fixture->accel_1, 0, false), NULL);

	/* Check that watermark and fifo full interrupts are 0 */
	zassert_ok(bmi160_emul_get_int_status_reg(fixture->accel_emul_1, 1, &int_status), NULL);
	zassert_true((int_status & (BMI160_INT_STATUS1_FFULL | BMI160_INT_STATUS1_FWM)) == 0, NULL);

	/* Check that the watermark was set to 0 */
	zassert_ok(bmi160_emul_get_watermark_reg(fixture->accel_emul_1, &watermark_reg_val), NULL);
	zassert_equal(0, watermark_reg_val, NULL);
}

ZTEST_F(sensor_accel, test_set_watermark_50)
{
	uint8_t int_status;
	uint8_t watermark_reg_val;

	/* Set the watermark to 50% */
	zassert_ok(sensor_fifo_set_watermark(fixture->accel_1, 50, false), NULL);

	/* Check that watermark and fifo full interrupts are 1 */
	zassert_ok(bmi160_emul_get_int_status_reg(fixture->accel_emul_1, 1, &int_status), NULL);
	zassert_equal((int_status & (BMI160_INT_STATUS1_FFULL | BMI160_INT_STATUS1_FWM)),
		      (BMI160_INT_STATUS1_FFULL | BMI160_INT_STATUS1_FWM), NULL);

	/* Check that the watermark was set to 0 */
	zassert_ok(bmi160_emul_get_watermark_reg(fixture->accel_emul_1, &watermark_reg_val), NULL);
	zassert_equal(128, watermark_reg_val, NULL);
}

static void *sensor_accel_setup(void)
{
	static struct sensor_accel_fixture fixture = {
		.accel_0 = DEVICE_DT_GET(DT_ALIAS(accel_0)),
		.accel_1 = DEVICE_DT_GET(DT_ALIAS(accel_1)),
		.accel_emul_0 = EMUL_DT_GET(DT_ALIAS(accel_0)),
		.accel_emul_1 = EMUL_DT_GET(DT_ALIAS(accel_1)),
	};

	return &fixture;
}

static void sensor_accel_before(void *f)
{
	struct sensor_accel_fixture *fixture = f;

	zassume_ok(bmi160_emul_set_bias(fixture->accel_emul_1, SENSOR_TYPE_ACCELEROMETER, 0, 0, 0),
		   NULL);
	zassume_ok(bmi160_emul_set_int_status_reg(fixture->accel_emul_1, 0, 0), NULL);
	zassume_ok(bmi160_emul_set_int_status_reg(fixture->accel_emul_1, 1, 0), NULL);
	zassume_ok(bmi160_emul_set_int_status_reg(fixture->accel_emul_1, 2, 0), NULL);
}
ZTEST_SUITE(sensor_accel, NULL, sensor_accel_setup, sensor_accel_before, NULL, NULL);
