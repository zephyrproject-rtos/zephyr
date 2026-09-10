/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

static void sensor_bmi160_setup_emulator(const struct device *dev, const struct emul *emulator)
{
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

	for (size_t i = 0; i < ARRAY_SIZE(values); ++i) {
		struct sensor_chan_spec chan_spec = {.chan_type = values[i].channel, .chan_idx = 0};

		zassert_ok(emul_sensor_backend_set_channel(emulator, chan_spec,
							   &values[i].value, 3));
	}
}

static void *bmi160_setup(void)
{
	static struct bmi160_fixture fixture = {
		.dev_spi = DEVICE_DT_GET(DT_ALIAS(accel_0)),
		.dev_i2c = DEVICE_DT_GET(DT_ALIAS(accel_1)),
		.emul_spi = EMUL_DT_GET(DT_ALIAS(accel_0)),
		.emul_i2c = EMUL_DT_GET(DT_ALIAS(accel_1)),
	};

	sensor_bmi160_setup_emulator(fixture.dev_i2c, fixture.emul_i2c);
	sensor_bmi160_setup_emulator(fixture.dev_spi, fixture.emul_spi);

	return &fixture;
}

static void bmi160_before(void *f)
{
	struct bmi160_fixture *fixture = (struct bmi160_fixture *)f;

	zassert_true(device_is_ready(fixture->dev_spi), "'%s' device is not ready",
		     fixture->dev_spi->name);
	zassert_true(device_is_ready(fixture->dev_i2c), "'%s' device is not ready",
		     fixture->dev_i2c->name);

	k_object_access_grant(fixture->dev_spi, k_current_get());
	k_object_access_grant(fixture->dev_i2c, k_current_get());
}

ZTEST_SUITE(bmi160, NULL, bmi160_setup, bmi160_before, NULL, NULL);
