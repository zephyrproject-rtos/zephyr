/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "icm42688_emul.h"
#include "icm42688_reg.h"

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
