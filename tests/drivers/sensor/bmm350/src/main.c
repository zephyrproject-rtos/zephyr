/*
 * Copyright (c) 2026 Meta Platforms
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/bmm350.h>
#include <zephyr/ztest.h>

#include "bmm350.h"
#include "bmm350_emul.h"

struct bmm350_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *bmm350_setup(void)
{
	static struct bmm350_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(bmm350)),
		.target = EMUL_DT_GET(DT_NODELABEL(bmm350)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void bmm350_before(void *f)
{
	struct bmm350_fixture *fixture = f;

	/* Start each test from a clean register file. */
	bmm350_emul_set_mag_raw(fixture->target, 0, 0, 0);
	bmm350_emul_set_temp_raw(fixture->target, 0);
}

ZTEST_SUITE(bmm350, NULL, bmm350_setup, bmm350_before, NULL, NULL);

/* The driver must have initialised against the emulated chip (chip-id, OTP, reset). */
ZTEST_F(bmm350, test_device_ready)
{
	zassert_true(device_is_ready(fixture->dev), "BMM350 device not ready");
}

/* Expected channel value (Gauss) for a given raw axis sample, with zero OTP trim. */
static double expected_mag_gauss(int32_t raw, int32_t coeff)
{
	int32_t micro_tesla = (int32_t)(((int64_t)raw * coeff) / BMM350_LSB_TO_UT_COEFF_DIV);

	return (double)micro_tesla / 100.0;
}

ZTEST_F(bmm350, test_fetch_mag)
{
	const int32_t raw_x = 10000;
	const int32_t raw_y = 20000;
	const int32_t raw_z = -15000;
	struct sensor_value val[3];

	bmm350_emul_set_mag_raw(fixture->target, raw_x, raw_y, raw_z);

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_MAGN_XYZ, val));

	zassert_within(sensor_value_to_double(&val[0]),
		       expected_mag_gauss(raw_x, BMM350_LSB_TO_UT_XY_COEFF), 0.01, "X mismatch");
	zassert_within(sensor_value_to_double(&val[1]),
		       expected_mag_gauss(raw_y, BMM350_LSB_TO_UT_XY_COEFF), 0.01, "Y mismatch");
	zassert_within(sensor_value_to_double(&val[2]),
		       expected_mag_gauss(raw_z, BMM350_LSB_TO_UT_Z_COEFF), 0.01, "Z mismatch");
}

ZTEST_F(bmm350, test_fetch_mag_single_channels)
{
	struct sensor_value val;

	bmm350_emul_set_mag_raw(fixture->target, 30000, 0, 0);
	zassert_ok(sensor_sample_fetch(fixture->dev));

	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_MAGN_X, &val));
	zassert_within(sensor_value_to_double(&val),
		       expected_mag_gauss(30000, BMM350_LSB_TO_UT_XY_COEFF), 0.01, "X mismatch");
}

ZTEST_F(bmm350, test_die_temp)
{
	/* raw 50490 -> 50490/10 - 25.49 = 25.00 degC */
	struct sensor_value val;

	bmm350_emul_set_temp_raw(fixture->target, 50490);

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_DIE_TEMP, &val));

	zassert_within(sensor_value_to_double(&val), 25.0, 0.05, "temperature mismatch: %d.%06d",
		       val.val1, val.val2);
}

ZTEST_F(bmm350, test_attr_sampling_frequency)
{
	struct sensor_value odr = {.val1 = 100, .val2 = 0};
	struct sensor_value read_back;

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_MAGN_XYZ,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &odr));
	zassert_ok(sensor_attr_get(fixture->dev, SENSOR_CHAN_MAGN_XYZ,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &read_back));

	zassert_equal(read_back.val1, 100, "expected 100 Hz, got %d.%06d", read_back.val1,
		      read_back.val2);
}

/*
 * Drive the full self-test PMU/forced-mode handshake against the emulator, which
 * models an excitation response on each axis. bmm350_self_test() reports the raw
 * deltas; here we check they exceed the datasheet minimum (260 uT).
 */
ZTEST_F(bmm350, test_self_test)
{
	struct bmm350_self_test_result result = {0};

	zassert_ok(bmm350_self_test(fixture->dev, &result));
	zassert_true(result.x_delta >= 260, "x_delta=%d too small", result.x_delta);
	zassert_true(result.y_delta >= 260, "y_delta=%d too small", result.y_delta);
}
