/*
 * Copyright (c) 2026 Chaogui Deng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "bmi323_emul.h"

/* The value the BMI323 reports in a data register when it has no sample. */
#define BMI323_NO_DATA ((int16_t)0x8000)

struct bmi323_fixture {
	const struct device *i2c_dev;
	const struct emul *i2c_target;
	const struct device *spi_dev;
	const struct emul *spi_target;
};

static void *bmi323_setup(void)
{
	static struct bmi323_fixture fixture = {
		.i2c_dev = DEVICE_DT_GET(DT_NODELABEL(bmi323_i2c)),
		.i2c_target = EMUL_DT_GET(DT_NODELABEL(bmi323_i2c)),
		.spi_dev = DEVICE_DT_GET(DT_NODELABEL(bmi323_spi)),
		.spi_target = EMUL_DT_GET(DT_NODELABEL(bmi323_spi)),
	};

	zassert_not_null(fixture.i2c_dev);
	zassert_not_null(fixture.i2c_target);
	zassert_not_null(fixture.spi_dev);
	zassert_not_null(fixture.spi_target);

	return &fixture;
}

ZTEST_SUITE(bmi323, NULL, bmi323_setup, NULL, NULL, NULL);

static void check_device_ready(const struct device *dev)
{
	zassert_true(device_is_ready(dev), "%s device not ready", dev->name);
}

/* A node that sets no properties gets the reset values: both sub-sensors off. */
static void check_dt_defaults(const struct device *dev)
{
	struct sensor_value value;

	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FEATURE_MASK, &value));
	zassert_equal(0, value.val1, "accelerometer should be disabled");
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FEATURE_MASK, &value));
	zassert_equal(0, value.val1, "gyroscope should be disabled");

	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
				   &value));
	zassert_equal(100, value.val1);
	zassert_equal(0, value.val2);
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &value));
	zassert_equal(8, value.val1);
	zassert_equal(0, value.val2);

	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
				   &value));
	zassert_equal(100, value.val1);
	zassert_equal(0, value.val2);
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &value));
	zassert_equal(2000, value.val1);
	zassert_equal(0, value.val2);
}

static void check_dt_config(const struct device *dev)
{
	struct sensor_value value;

	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FEATURE_MASK, &value));
	zassert_equal(1, value.val1, "accelerometer should be enabled");
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FEATURE_MASK, &value));
	zassert_equal(1, value.val1, "gyroscope should be enabled");

	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
				   &value));
	zassert_equal(200, value.val1);
	zassert_equal(0, value.val2);

	zassert_ok(
		sensor_attr_get(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &value));
	zassert_equal(50, value.val1);
	zassert_equal(0, value.val2);

	/* Ranges are not described in devicetree, so they stay at reset values. */
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &value));
	zassert_equal(8, value.val1);
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &value));
	zassert_equal(2000, value.val1);
}

static void check_attributes(const struct device *dev)
{
	struct sensor_value set;
	struct sensor_value get;

	set = (struct sensor_value){.val1 = 4};
	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &set));
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &get));
	zassert_equal(4, get.val1);
	zassert_equal(0, get.val2);

	set = (struct sensor_value){.val1 = 200};
	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
				   &set));
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
				   &get));
	zassert_equal(200, get.val1);
	zassert_equal(0, get.val2);

	set = (struct sensor_value){.val1 = 500};
	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &set));
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &get));
	zassert_equal(500, get.val1);
	zassert_equal(0, get.val2);
}

static void check_invalid_samples(const struct device *dev)
{
	zassert_equal(-ENODATA, sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ));
	zassert_equal(-ENODATA, sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ));
	zassert_equal(-ENODATA, sensor_sample_fetch_chan(dev, SENSOR_CHAN_DIE_TEMP));
	zassert_equal(-ENODATA, sensor_sample_fetch(dev));
}

static void check_per_axis_channels(const struct device *dev, const struct emul *target)
{
	static const enum sensor_channel accel_axes[] = {SENSOR_CHAN_ACCEL_X, SENSOR_CHAN_ACCEL_Y,
							 SENSOR_CHAN_ACCEL_Z};
	static const enum sensor_channel gyro_axes[] = {SENSOR_CHAN_GYRO_X, SENSOR_CHAN_GYRO_Y,
							SENSOR_CHAN_GYRO_Z};
	struct sensor_value vector[3];
	struct sensor_value axis;

	bmi323_emul_set_accel_raw(target, 1000, -2000, 3000);
	bmi323_emul_set_gyro_raw(target, -300, 400, -500);
	zassert_ok(sensor_sample_fetch(dev));

	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, vector));
	for (size_t i = 0; i < ARRAY_SIZE(accel_axes); i++) {
		zassert_ok(sensor_channel_get(dev, accel_axes[i], &axis));
		zassert_equal(sensor_value_to_micro(&vector[i]), sensor_value_to_micro(&axis),
			      "accelerometer axis %zu disagrees with the vector", i);
	}

	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, vector));
	for (size_t i = 0; i < ARRAY_SIZE(gyro_axes); i++) {
		zassert_ok(sensor_channel_get(dev, gyro_axes[i], &axis));
		zassert_equal(sensor_value_to_micro(&vector[i]), sensor_value_to_micro(&axis),
			      "gyroscope axis %zu disagrees with the vector", i);
	}

	zassert_ok(sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_Z));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &axis));

	zassert_equal(-ENOTSUP, sensor_sample_fetch_chan(dev, SENSOR_CHAN_MAGN_XYZ));
	zassert_equal(-ENOTSUP, sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, &axis));
}

static void check_fetch_all_partial(const struct device *dev, const struct emul *target)
{
	struct sensor_value values[3];

	bmi323_emul_set_accel_raw(target, 100, 200, 300);
	bmi323_emul_set_gyro_raw(target, 1, 2, 3);
	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, values));

	/* Take the gyroscope away; the accelerometer fetch must still succeed. */
	bmi323_emul_set_gyro_raw(target, BMI323_NO_DATA, BMI323_NO_DATA, BMI323_NO_DATA);
	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, values));
	zassert_equal(-ENODATA, sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, values));
}

static void check_fetch_temperature(const struct device *dev, const struct emul *target)
{
	struct sensor_value value;

	/* 23 degC + 512 * 0.001953 degC/LSB = 23.999936 degC. */
	bmi323_emul_set_temperature_raw(target, 512);
	zassert_ok(sensor_sample_fetch_chan(dev, SENSOR_CHAN_DIE_TEMP));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &value));
	zassert_equal(23, value.val1);
	zassert_equal(999936, value.val2);
}

static void check_fetch_motion(const struct device *dev, const struct emul *target)
{
	struct sensor_value values[3];

	bmi323_emul_set_accel_raw(target, 1, -1, 0);
	zassert_ok(sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, values));
	zassert_true(sensor_value_to_micro(&values[0]) > 0);
	zassert_true(sensor_value_to_micro(&values[1]) < 0);
	zassert_equal(0, sensor_value_to_micro(&values[2]));

	bmi323_emul_set_gyro_raw(target, -1, 0, 1);
	zassert_ok(sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, values));
	zassert_true(sensor_value_to_micro(&values[0]) < 0);
	zassert_equal(0, sensor_value_to_micro(&values[1]));
	zassert_true(sensor_value_to_micro(&values[2]) > 0);
}

ZTEST_F(bmi323, test_i2c)
{
	check_device_ready(fixture->i2c_dev);
	check_dt_config(fixture->i2c_dev);
	check_invalid_samples(fixture->i2c_dev);
	check_attributes(fixture->i2c_dev);
	check_fetch_temperature(fixture->i2c_dev, fixture->i2c_target);
	check_fetch_motion(fixture->i2c_dev, fixture->i2c_target);
	check_per_axis_channels(fixture->i2c_dev, fixture->i2c_target);
	check_fetch_all_partial(fixture->i2c_dev, fixture->i2c_target);
}

ZTEST_F(bmi323, test_spi)
{
	check_device_ready(fixture->spi_dev);
	check_dt_defaults(fixture->spi_dev);
	check_invalid_samples(fixture->spi_dev);
	check_attributes(fixture->spi_dev);
	check_fetch_temperature(fixture->spi_dev, fixture->spi_target);
	check_fetch_motion(fixture->spi_dev, fixture->spi_target);
	check_per_axis_channels(fixture->spi_dev, fixture->spi_target);
	check_fetch_all_partial(fixture->spi_dev, fixture->spi_target);
}
