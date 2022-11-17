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

static void *max17048_setup(void)
{
	static ZTEST_DMEM struct max17048_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(maxim_max17048);

	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

ZTEST_USER_F(max17048, test_get_all_props_failed_returns_negative)
{
	struct fuel_gauge_get_property props[] = {
		{
			/* Invalid property */
			.property_type = FUEL_GAUGE_PROP_MAX,
		},
	};

	int ret = fuel_gauge_get_prop(fixture->dev, props, ARRAY_SIZE(props));

	zassert_equal(props[0].status, -ENOTSUP, "Getting bad property %d has a good status.",
		      props[0].property_type);

	zassert_true(ret < 0);
}

ZTEST_USER_F(max17048, test_get_some_props_failed_returns_failed_prop_count)
{
	struct fuel_gauge_get_property props[] = {
		{
			/* First invalid property */
			.property_type = FUEL_GAUGE_PROP_MAX,
		},
		{
			/* Second invalid property */
			.property_type = FUEL_GAUGE_PROP_MAX,
		},
		{
			/* Valid property */
			.property_type = FUEL_GAUGE_VOLTAGE,
		},

	};

	int ret = fuel_gauge_get_prop(fixture->dev, props, ARRAY_SIZE(props));

	zassert_equal(props[0].status, -ENOTSUP, "Getting bad property %d has a good status.",
		      props[0].property_type);

	zassert_equal(props[1].status, -ENOTSUP, "Getting bad property %d has a good status.",
		      props[1].property_type);

	zassert_ok(props[2].status, "Property %d getting %d has a bad status.", 2,
		   props[2].property_type);

	zassert_equal(ret, 2);
}


ZTEST_USER_F(max17048, test_get_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	struct fuel_gauge_get_property props[] = {
		{
			.property_type = FUEL_GAUGE_RUNTIME_TO_EMPTY,
		},
		{
			.property_type = FUEL_GAUGE_RUNTIME_TO_FULL,
		},
		{
			.property_type = FUEL_GAUGE_STATE_OF_CHARGE,
		},
		{
			.property_type = FUEL_GAUGE_VOLTAGE,
		}
	};

	int ret = fuel_gauge_get_prop(fixture->dev, props, ARRAY_SIZE(props));

	for (int i = 0; i < ARRAY_SIZE(props); i++) {
		zassert_ok(props[i].status, "Property %d getting %d has a bad status.", i,
			   props[i].property_type);
	}

	zassert_ok(ret);
}


ZTEST_SUITE(max17048, NULL, max17048_setup, NULL, NULL, NULL);
