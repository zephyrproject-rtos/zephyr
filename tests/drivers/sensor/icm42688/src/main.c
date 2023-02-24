/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include "icm42688_emul.h"
#include "icm42688_reg.h"

#define NODE DT_NODELABEL(icm42688)

DEFINE_FFF_GLOBALS;

struct icm42688_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *icm42688_setup(void)
{
	static struct icm42688_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(icm42688)),
		.target = EMUL_DT_GET(DT_NODELABEL(icm42688)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

ZTEST_SUITE(icm42688, NULL, icm42688_setup, NULL, NULL, NULL);

ZTEST_F(icm42688, test_fetch_fail_no_ready_data)
{
	uint8_t status = 0;

	icm42688_emul_set_reg(fixture->target, REG_INT_STATUS, &status, 1);
	zassert_equal(-EBUSY, sensor_sample_fetch(fixture->dev));
}

static void test_fetch_temp_mc(const struct icm42688_fixture *fixture, int16_t temperature_mc)
{
	struct sensor_value value;
	int64_t expected_uc;
	int64_t actual_uc;
	int16_t temperature_reg;
	uint8_t buffer[2];

	/* Set the INT_STATUS register to show we have data */
	buffer[0] = BIT_INT_STATUS_DATA_RDY;
	icm42688_emul_set_reg(fixture->target, REG_INT_STATUS, buffer, 1);

	/*
	 * Set the temperature data to 22.5C via:
	 *   reg_val / 132.48 + 25
	 */
	temperature_reg = ((temperature_mc - 25000) * 13248) / 100000;
	buffer[0] = (temperature_reg >> 8) & GENMASK(7, 0);
	buffer[1] = temperature_reg & GENMASK(7, 0);
	icm42688_emul_set_reg(fixture->target, REG_TEMP_DATA1, buffer, 2);

	/* Fetch the data */
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_DIE_TEMP, &value));

	/* Assert data is within 5 milli-C of the tested temperature */
	expected_uc = temperature_mc * INT64_C(1000);
	actual_uc = sensor_value_to_micro(&value);
	zassert_within(expected_uc, actual_uc, INT64_C(5000),
		       "Expected %" PRIi64 "uC, got %" PRIi64 "uC", expected_uc, actual_uc);
}

ZTEST_F(icm42688, test_fetch_temp)
{
	/* Test 22.5C */
	test_fetch_temp_mc(fixture, 22500);
	/* Test -3.175C */
	test_fetch_temp_mc(fixture, -3175);
}

static void test_fetch_accel_with_range(const struct icm42688_fixture *fixture,
					int16_t accel_range_g, const int16_t accel_percent[3])
{
	struct sensor_value values[3];
	int32_t expect_ug;
	int32_t actual_ug;
	uint8_t register_buffer[6];

	/* Se the INT_STATUS register to show we have data */
	register_buffer[0] = BIT_INT_STATUS_DATA_RDY;
	icm42688_emul_set_reg(fixture->target, REG_INT_STATUS, register_buffer, 1);

	/* Set accel range */
	sensor_g_to_ms2(accel_range_g, &values[0]);
	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE,
				   &values[0]));

	/* Set the accel data accel_percent * accel_range_g */
	for (int i = 0; i < 3; ++i) {
		register_buffer[i * 2] = (accel_percent[i] >> 8) & GENMASK(7, 0);
		register_buffer[i * 2 + 1] = accel_percent[i] & GENMASK(7, 0);
	}
	icm42688_emul_set_reg(fixture->target, REG_ACCEL_DATA_X1, register_buffer, 6);

	/* Fetch the data */
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_ACCEL_XYZ, values));

	/* Assert the data is within 0.005g (0.05m/s2) */
	actual_ug = sensor_ms2_to_ug(&values[0]);
	expect_ug = (int32_t)(accel_percent[0] * INT64_C(1000000) * accel_range_g / INT16_MAX);
	zassert_within(expect_ug, actual_ug, INT32_C(5000),
		       "Expected %" PRIi32 " ug, got X=%" PRIi32 " ug", expect_ug, actual_ug);

	actual_ug = sensor_ms2_to_ug(&values[1]);
	expect_ug = (int32_t)(accel_percent[1] * INT64_C(1000000) * accel_range_g / INT16_MAX);
	zassert_within(expect_ug, actual_ug, INT32_C(5000),
		       "Expected %" PRIi32 " ug, got X=%" PRIi32 " ug", expect_ug, actual_ug);

	actual_ug = sensor_ms2_to_ug(&values[2]);
	expect_ug = (int32_t)(accel_percent[2] * INT64_C(1000000) * accel_range_g / INT16_MAX);
	zassert_within(expect_ug, actual_ug, INT32_C(5000),
		       "Expected %" PRIi32 " ug, got X=%" PRIi32 " ug", expect_ug, actual_ug);
}

ZTEST_F(icm42688, test_fetch_accel)
{
	/* Use (0.25, -0.33.., 0.91) * range for testing accel values */
	const int16_t accel_percent[3] = {
		INT16_MAX / INT16_C(4),
		INT16_MIN / INT16_C(3),
		(int16_t)((INT16_MAX * INT32_C(91)) / INT32_C(100)),
	};

	test_fetch_accel_with_range(fixture, 2, accel_percent);
	test_fetch_accel_with_range(fixture, 4, accel_percent);
	test_fetch_accel_with_range(fixture, 8, accel_percent);
	test_fetch_accel_with_range(fixture, 16, accel_percent);
}

static void test_fetch_gyro_with_range(const struct icm42688_fixture *fixture, int32_t scale_mdps,
				       const int16_t gyro_percent[3])
{
	/* Set the epsilon to 0.075% of the scale */
	const int32_t epsilon_10udps = scale_mdps * 75 / 1000;
	struct sensor_value values[3];
	int32_t expect_10udps;
	int32_t actual_10udps;
	uint8_t register_buffer[6];

	/* Se the INT_STATUS register to show we have data */
	register_buffer[0] = BIT_INT_STATUS_DATA_RDY;
	icm42688_emul_set_reg(fixture->target, REG_INT_STATUS, register_buffer, 1);

	/* Set gyro range */
	sensor_degrees_to_rad((scale_mdps / 1000) + (scale_mdps % 1000 == 0 ? 0 : 1), &values[0]);
	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE,
				   &values[0]));

	/* Set the gyro data to gyro_percent */
	for (int i = 0; i < 3; ++i) {
		register_buffer[i * 2] = (gyro_percent[i] >> 8) & GENMASK(7, 0);
		register_buffer[i * 2 + 1] = gyro_percent[i] & GENMASK(7, 0);
	}
	icm42688_emul_set_reg(fixture->target, REG_GYRO_DATA_X1, register_buffer, 6);

	/* Fetch the data */
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_GYRO_XYZ, values));

	/* Assert the data is within 0.5 d/s (0.001 rad/s) */
	actual_10udps = sensor_rad_to_10udegrees(&values[0]);
	expect_10udps = (int32_t)(gyro_percent[0] * INT64_C(100) * scale_mdps / INT16_MAX);
	zassert_within(expect_10udps, actual_10udps, epsilon_10udps,
		       "[scale=%" PRIi32 "md/s] Expected %" PRIi32 " 10ud/s, got %" PRIi32
		       " 10ud/s",
		       scale_mdps, expect_10udps, actual_10udps);

	actual_10udps = sensor_rad_to_10udegrees(&values[1]);
	expect_10udps = (int32_t)(gyro_percent[1] * INT64_C(100) * scale_mdps / INT16_MAX);
	zassert_within(expect_10udps, actual_10udps, epsilon_10udps,
		       "[scale=%" PRIi32 "md/s] Expected %" PRIi32 " 10ud/s, got %" PRIi32
		       " 10ud/s, ",
		       scale_mdps, expect_10udps, actual_10udps);

	actual_10udps = sensor_rad_to_10udegrees(&values[2]);
	expect_10udps = (int32_t)(gyro_percent[2] * INT64_C(100) * scale_mdps / INT16_MAX);
	zassert_within(expect_10udps, actual_10udps, epsilon_10udps,
		       "[scale=%" PRIi32 "md/s] Expected %" PRIi32 " 10ud/s, got %" PRIi32
		       " 10ud/s",
		       scale_mdps, expect_10udps, actual_10udps);
}

ZTEST_F(icm42688, test_fetch_gyro)
{
	/* Use (0.15, 0.68, -0.22) * range for testing gyro values */
	const int16_t gyro_percent[3] = {
		(int16_t)((INT16_MAX * INT32_C(15)) / INT32_C(100)),
		(int16_t)((INT16_MAX * INT32_C(68)) / INT32_C(100)),
		(int16_t)((INT16_MAX * INT32_C(-22)) / INT32_C(100)),
	};

	test_fetch_gyro_with_range(fixture, 2000000, gyro_percent);
	test_fetch_gyro_with_range(fixture, 1000000, gyro_percent);
	test_fetch_gyro_with_range(fixture, 500000, gyro_percent);
	test_fetch_gyro_with_range(fixture, 250000, gyro_percent);
	test_fetch_gyro_with_range(fixture, 125000, gyro_percent);
	test_fetch_gyro_with_range(fixture, 62500, gyro_percent);
	test_fetch_gyro_with_range(fixture, 31250, gyro_percent);
	test_fetch_gyro_with_range(fixture, 15625, gyro_percent);
}

FAKE_VOID_FUNC(test_interrupt_trigger_handler, const struct device*, const struct sensor_trigger*);

ZTEST_F(icm42688, test_interrupt)
{
	const struct gpio_dt_spec spec = GPIO_DT_SPEC_GET(NODE, int_gpios);
	const struct sensor_trigger trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	RESET_FAKE(test_interrupt_trigger_handler);
	sensor_trigger_set(fixture->dev, &trigger, test_interrupt_trigger_handler);

	/* Toggle the GPIO */
	gpio_emul_input_set(spec.port, spec.pin, 0);
	k_msleep(5);
	gpio_emul_input_set(spec.port, spec.pin, 1);
	k_msleep(5);

	/* Verify the handler was called */
	zassert_equal(test_interrupt_trigger_handler_fake.call_count, 1);
}
