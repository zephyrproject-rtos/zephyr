/*
 * Copyright (c) 2023 Alvaro Garcia Gomez <maxpowel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

struct sy24561_fixture {
	const struct device *dev;
	const struct fuel_gauge_driver_api *api;
};

void emul_sy24561_set_crate_status(int value);

static void *sy24561_setup(void)
{
	static ZTEST_DMEM struct sy24561_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(silergy_sy24561);
	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

ZTEST_USER_F(sy24561, test_get_some_props_failed_returns_bad_status)
{
	fuel_gauge_prop_t prop_types[] = {
		/* First invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Second invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Valid property */
		FUEL_GAUGE_VOLTAGE,
	};
	union fuel_gauge_prop_val props[ARRAY_SIZE(prop_types)];

	int ret = fuel_gauge_get_props(fixture->dev, prop_types, props, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP, "Getting bad property has a good status.");
}

ZTEST_USER_F(sy24561, test_get_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	fuel_gauge_prop_t prop_types[] = {
		FUEL_GAUGE_VOLTAGE,
		FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
		FUEL_GAUGE_STATUS,
		FUEL_GAUGE_CURRENT_DIRECTION,
	};

	union fuel_gauge_prop_val props[ARRAY_SIZE(prop_types)];

	zassert_ok(fuel_gauge_get_props(fixture->dev, prop_types, props, ARRAY_SIZE(props)));
	zassert_equal(props[0].voltage, 3199000);
	zassert_equal(props[1].relative_state_of_charge, 74);
	zassert_equal(props[2].fg_status, 0);
	zassert_equal(props[3].current_direction, 0);
}

ZTEST_USER_F(sy24561, test_out_of_range_temperature_are_cropped)
{
	union fuel_gauge_prop_val val;
	int ret = 0;

	val.temperature = 0;
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_TEMPERATURE,
				  val); /* A warning is triggered but it should pass */
	zassert_equal(ret, 0, "Setting too low temperature has good status");

	val.temperature = 0xffff;
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_TEMPERATURE,
				  val); /* A warning is triggered but it should pass */
	zassert_equal(ret, 0, "Setting too high temperature has good status");
}

ZTEST_USER_F(sy24561, test_out_of_range_alarm_threshold_are_cropped)
{
	union fuel_gauge_prop_val val;
	int ret = 0;

	val.state_of_charge_alarm = 0u;
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_STATE_OF_CHARGE_ALARM,
				  val); /* A warning is triggered but it should pass */
	zassert_equal(ret, 0, "Setting too low alarm threshold has good status");

	val.state_of_charge_alarm = 0xffu;
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_STATE_OF_CHARGE_ALARM,
				  val); /* A warning is triggered but it should pass */
	zassert_equal(ret, 0, "Setting too high alarm threshold has good status");
}

ZTEST_SUITE(sy24561, NULL, sy24561_setup, NULL, NULL, NULL);
