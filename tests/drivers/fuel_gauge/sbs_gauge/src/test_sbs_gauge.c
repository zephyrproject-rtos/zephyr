/*
 * Copyright 2022 Google LLC
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

struct sbs_gauge_new_api_fixture {
	const struct device *dev;
	const struct battery_driver_api *api;
};

static void *sbs_gauge_new_api_setup(void)
{

	static struct sbs_gauge_new_api_fixture fixture;
	const struct device *dev = DEVICE_DT_GET_ANY(sbs_sbs_gauge_new_api);
	const struct battery_driver_api *api = dev->api;

	fixture.dev = dev;
	fixture.api = api;

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");
	zassert_not_null(api);

	return &fixture;
}

ZTEST_F(sbs_gauge_new_api, test_implemented_apis)
{
	zassert_not_equal(NULL, fixture->api->get_property);
}

ZTEST_F(sbs_gauge_new_api, test_get_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	struct fuel_gauge_get_property props[] = {
		{
			.property_type = FUEL_GAUGE_VOLTAGE,
		},
		{
			.property_type = FUEL_GAUGE_CURRENT,
		},
		{
			.property_type = FUEL_GAUGE_AVG_CURRENT,
		},
		{
			.property_type = FUEL_GAUGE_TEMPERATURE,
		},
		{
			.property_type = FUEL_GAUGE_STATE_OF_CHARGE,
		},
		{
			.property_type = FUEL_GAUGE_RUNTIME_TO_FULL,
		},
		{
			.property_type = FUEL_GAUGE_RUNTIME_TO_EMPTY,
		},
		{
			.property_type = FUEL_GAUGE_REMAINING_CAPACITY,
		},
		{
			.property_type = FUEL_GAUGE_FULL_CHARGE_CAPACITY,
		},
		{
			.property_type = FUEL_GAUGE_CYCLE_COUNT,
		},
	};

	int ret = fixture->api->get_property(fixture->dev, props, ARRAY_SIZE(props));

	for (int i = 0; i < ARRAY_SIZE(props); i++) {
		zassert_ok(props[i].status, "Property %d getting %d has a bad status.", i,
			   props[i].property_type);
	}

	zassert_ok(ret);
}

ZTEST_SUITE(sbs_gauge_new_api, NULL, sbs_gauge_new_api_setup, NULL, NULL, NULL);
