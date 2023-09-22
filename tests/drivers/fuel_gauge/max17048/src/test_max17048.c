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

struct max17048_fixture {
	const struct device *dev;
	const struct fuel_gauge_driver_api *api;
};

void emul_max17048_set_crate_status(int value);

static void *max17048_setup(void)
{
	static ZTEST_DMEM struct max17048_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(maxim_max17048);
	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

ZTEST_USER_F(max17048, test_get_some_props_failed_returns_bad_status)
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

ZTEST_USER_F(max17048, test_get_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	fuel_gauge_prop_t prop_types[] = {
		FUEL_GAUGE_VOLTAGE,
		FUEL_GAUGE_RUNTIME_TO_EMPTY,
		FUEL_GAUGE_RUNTIME_TO_FULL,
		FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
	};

	union fuel_gauge_prop_val props[ARRAY_SIZE(prop_types)];

	zassert_ok(fuel_gauge_get_props(fixture->dev, prop_types, props, ARRAY_SIZE(props)));
}

ZTEST_USER_F(max17048, test_current_rate_zero)
{
	/* Test when crate is 0, which is a special case */

	fuel_gauge_prop_t prop_types[] = {
		FUEL_GAUGE_RUNTIME_TO_EMPTY,
		FUEL_GAUGE_RUNTIME_TO_FULL,
	};
	union fuel_gauge_prop_val props[ARRAY_SIZE(prop_types)];

	/** Null value, not charging either discharging. If not handled correctly,
	 * it will cause a division by zero
	 */
	emul_max17048_set_crate_status(0);
	int ret = fuel_gauge_get_props(fixture->dev, prop_types, props, ARRAY_SIZE(props));

	zassert_equal(props[0].runtime_to_empty, 0, "Runtime to empty is %d but it should be 0.",
		      props[0].runtime_to_full);
	zassert_equal(props[1].runtime_to_full, 0, "Runtime to full is %d but it should be 0.",
		      props[1].runtime_to_full);

	zassert_ok(ret);
	/* Return value to the original state */
	emul_max17048_set_crate_status(0x4000);
}

ZTEST_SUITE(max17048, NULL, max17048_setup, NULL, NULL, NULL);
