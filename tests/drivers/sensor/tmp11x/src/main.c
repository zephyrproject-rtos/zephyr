/*
 * Copyright (c) 2026 Junseo Pyun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "tmp11x_emul.h"
#include "tmp11x.h"

struct tmp11x_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *tmp11x_setup(void)
{
	static struct tmp11x_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(tmp11x)),
		.target = EMUL_DT_GET(DT_NODELABEL(tmp11x)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);

	return &fixture;
}

static void tmp11x_before(void *f)
{
	struct tmp11x_fixture *fixture = f;

	tmp11x_emul_write_reg(fixture->target, TMP11X_REG_TEMP, 0);
}

static void check_fetch_temp(const struct device *dev, const struct emul *target, int16_t raw_temp)
{
	struct sensor_value val;
	double expected;

	/* raw 8000 -> 8000 / 128.0 = -256 degC */

	tmp11x_emul_write_reg(target, TMP11X_REG_TEMP, raw_temp);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val));

	expected = raw_temp * 0.0078125;

	zassert_within(sensor_value_to_double(&val), expected, 0.01,
		       "temperature mismatch: %d.%06d", val.val1, val.val2);
}

ZTEST_SUITE(tmp11x, NULL, tmp11x_setup, tmp11x_before, NULL, NULL);

ZTEST_F(tmp11x, test_device_ready)
{
	zassert_true(device_is_ready(fixture->dev), "TMP11x device not ready");
}

ZTEST_F(tmp11x, test_attr_set_sampling_frequency)
{
	struct sensor_value odr = {.val1 = 8, .val2 = 0};

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &odr));
}

ZTEST_F(tmp11x, test_fetch_temp_positive)
{
	int16_t raw = (int16_t)0x0C80;

	check_fetch_temp(fixture->dev, fixture->target, raw);
}

ZTEST_F(tmp11x, test_fetch_temp_negative)
{
	int16_t raw = (int16_t)0xF600;

	check_fetch_temp(fixture->dev, fixture->target, raw);
}
