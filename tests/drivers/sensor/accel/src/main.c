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
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

struct sensor_accel_fixture {
	const struct device *accel_spi;
	const struct device *accel_i2c;
	const struct emul *accel_emul_spi;
	const struct emul *accel_emul_i2c;
};

static enum sensor_channel channel[] = {
	SENSOR_CHAN_ACCEL_X, SENSOR_CHAN_ACCEL_Y, SENSOR_CHAN_ACCEL_Z,
	SENSOR_CHAN_GYRO_X,  SENSOR_CHAN_GYRO_Y,  SENSOR_CHAN_GYRO_Z,
};

static int32_t compute_epsilon_micro(q31_t value, int8_t shift)
{
	int64_t intermediate = value;

	if (shift > 0) {
		intermediate <<= shift;
	} else if (shift < 0) {
		intermediate >>= -shift;
	}

	intermediate = intermediate * INT64_C(1000000) / INT32_MAX;
	return CLAMP(intermediate, INT32_MIN, INT32_MAX);
}

static void test_sensor_accel_basic(const struct device *dev, const struct emul *emulator)
{
	q31_t accel_ranges[3];
	q31_t gyro_ranges[3];
	int8_t accel_shift;
	int8_t gyro_shift;

	zassert_ok(sensor_sample_fetch(dev), "fail to fetch sample");
	zassert_ok(emul_sensor_backend_get_sample_range(emulator, SENSOR_CHAN_ACCEL_XYZ,
							&accel_ranges[0], &accel_ranges[1],
							&accel_ranges[2], &accel_shift));
	zassert_ok(emul_sensor_backend_get_sample_range(emulator, SENSOR_CHAN_GYRO_XYZ,
							&gyro_ranges[0], &gyro_ranges[1],
							&gyro_ranges[2], &gyro_shift));

	int32_t accel_epsilon = compute_epsilon_micro(accel_ranges[2], accel_shift);
	int32_t gyro_epsilon = compute_epsilon_micro(gyro_ranges[2], gyro_shift);

	for (int i = 0; i < ARRAY_SIZE(channel); i++) {
		struct sensor_value val;
		int64_t micro_val;
		int32_t epsilon = (i < 3) ? accel_epsilon : gyro_epsilon;

		zassert_ok(sensor_channel_get(dev, channel[i], &val), "fail to get channel");

		micro_val = sensor_value_to_micro(&val);
		zassert_within(i * INT64_C(1000000), micro_val, epsilon,
			       "%d. expected %" PRIi64 " to be within %d of %" PRIi64, i,
			       i * INT64_C(1000000), epsilon, micro_val);
	}
}

/* Run all of our tests on an accelerometer device with the given label */
static void run_tests_on_accel(const struct device *accel)
{
	zassert_true(device_is_ready(accel), "Accelerometer device is not ready");

	PRINT("Running tests on '%s'\n", accel->name);
	k_object_access_grant(accel, k_current_get());
}

ZTEST_USER_F(sensor_accel, test_sensor_accel_basic_spi)
{
	run_tests_on_accel(fixture->accel_spi);
	test_sensor_accel_basic(fixture->accel_spi, fixture->accel_emul_spi);
}

ZTEST_USER_F(sensor_accel, test_sensor_accel_basic_i2c)
{
	if (fixture->accel_i2c == NULL) {
		ztest_test_skip();
	}

	run_tests_on_accel(fixture->accel_i2c);
	test_sensor_accel_basic(fixture->accel_i2c, fixture->accel_emul_i2c);
}

static void sensor_accel_setup_emulator(const struct device *dev, const struct emul *accel_emul)
{
	if (accel_emul == NULL) {
		return;
	}

	static struct {
		enum sensor_channel channel;
		q31_t value;
	} values[] = {
		{SENSOR_CHAN_ACCEL_X, 0},       {SENSOR_CHAN_ACCEL_Y, 1 << 28},
		{SENSOR_CHAN_ACCEL_Z, 2 << 28}, {SENSOR_CHAN_GYRO_X, 3 << 28},
		{SENSOR_CHAN_GYRO_Y, 4 << 28},  {SENSOR_CHAN_GYRO_Z, 5 << 28},
	};
	static struct sensor_value scale;

	/* 4g */
	scale.val1 = 39;
	scale.val2 = 226600;
	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &scale));

	/* 125 deg/s */
	scale.val1 = 2;
	scale.val2 = 181661;
	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &scale));

	for (int i = 0; i < ARRAY_SIZE(values); ++i) {
		zassert_ok(emul_sensor_backend_set_channel(accel_emul, values[i].channel,
							   &values[i].value, 3));
	}
}

static void *sensor_accel_setup(void)
{
	static struct sensor_accel_fixture fixture = {
		.accel_spi = DEVICE_DT_GET(DT_ALIAS(accel_0)),
		.accel_i2c = DEVICE_DT_GET_OR_NULL(DT_ALIAS(accel_1)),
		.accel_emul_spi = EMUL_DT_GET(DT_ALIAS(accel_0)),
		.accel_emul_i2c = EMUL_DT_GET_OR_NULL(DT_ALIAS(accel_1)),
	};

	sensor_accel_setup_emulator(fixture.accel_spi, fixture.accel_emul_spi);
	sensor_accel_setup_emulator(fixture.accel_i2c, fixture.accel_emul_i2c);

	return &fixture;
}

ZTEST_SUITE(sensor_accel, NULL, sensor_accel_setup, NULL, NULL, NULL);
