/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "akm09918c_emul.h"
#include "akm09918c_reg.h"

struct akm09918c_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *akm09918c_setup(void)
{
	static struct akm09918c_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(akm09918c)),
		.target = EMUL_DT_GET(DT_NODELABEL(akm09918c)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void akm09918c_before(void *f)
{
	struct akm09918c_fixture *fixture = f;

	akm09918c_emul_reset(fixture->target);
}

ZTEST_SUITE(akm09918c, NULL, akm09918c_setup, akm09918c_before, NULL, NULL);

ZTEST_F(akm09918c, test_fetch_fail_no_ready_data)
{
	uint8_t status = 0;

	akm09918c_emul_set_reg(fixture->target, AKM09918C_REG_ST1, &status, 1);
	zassert_equal(-EBUSY, sensor_sample_fetch(fixture->dev));
}

static void test_fetch_magnetic_field(const struct akm09918c_fixture *fixture,
				      const int16_t magn_percent[3])
{
	struct sensor_value values[3];
	int64_t expect_ugauss;
	int64_t actual_ugauss;
	uint8_t register_buffer[6];

	/* Set the ST1 register to show we have data */
	register_buffer[0] = AKM09918C_ST1_DRDY;
	akm09918c_emul_set_reg(fixture->target, AKM09918C_REG_ST1, register_buffer, 1);

	/* Set the data magn_percent * range */
	for (int i = 0; i < 3; ++i) {
		register_buffer[i * 2 + 1] = (magn_percent[i] >> 8) & GENMASK(7, 0);
		register_buffer[i * 2] = magn_percent[i] & GENMASK(7, 0);
	}
	akm09918c_emul_set_reg(fixture->target, AKM09918C_REG_HXL, register_buffer, 6);

	/* Fetch the data */
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_MAGN_XYZ, values));

	/* Assert the data is within 0.000005 Gauss */
	actual_ugauss = values[0].val1 * INT64_C(1000000) + values[0].val2;
	expect_ugauss = magn_percent[0] * INT64_C(500);
	zassert_within(expect_ugauss, actual_ugauss, INT64_C(5),
		       "(X) expected %" PRIi64 " micro-gauss, got %" PRIi64 " micro-gauss",
		       expect_ugauss, actual_ugauss);

	actual_ugauss = values[1].val1 * INT64_C(1000000) + values[1].val2;
	expect_ugauss = magn_percent[1] * INT64_C(500);
	zassert_within(expect_ugauss, actual_ugauss, INT64_C(5),
		       "(Y) expected %" PRIi64 " micro-gauss, got %" PRIi64 " micro-gauss",
		       expect_ugauss, actual_ugauss);

	actual_ugauss = values[2].val1 * INT64_C(1000000) + values[2].val2;
	expect_ugauss = magn_percent[2] * INT64_C(500);
	zassert_within(expect_ugauss, actual_ugauss, INT64_C(5),
		       "(Z) expected %" PRIi64 " micro-gauss, got %" PRIi64 " micro-gauss",
		       expect_ugauss, actual_ugauss);
}

ZTEST_F(akm09918c, test_fetch_magn)
{
	/* Use (0.25, -0.33..., 0.91) as the factors */
	const int16_t magn_percent[3] = {
		INT16_C(32752) / INT16_C(4),
		INT16_C(-32751) / INT16_C(3),
		(int16_t)(INT16_C(32752) * INT32_C(91) / INT32_C(100)),
	};

	test_fetch_magnetic_field(fixture, magn_percent);
}
