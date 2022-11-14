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

ZTEST_F(sbs_gauge_new_api, test_voltage_read)
{
	struct fuel_gauge_get_property prop = {
		.property_type = FUEL_GAUGE_VOLTAGE,
	};
	zassert_ok(fixture->api->get_property(fixture->dev, &prop, 1));
}

ZTEST_SUITE(sbs_gauge_new_api, NULL, sbs_gauge_new_api_setup, NULL, NULL, NULL);
