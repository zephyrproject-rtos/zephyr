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

ZTEST_F(sbs_gauge_new_api, test_get_all_props_failed_returns_negative)
{
	struct fuel_gauge_get_property props[] = {
		{
			/* Invalid property */
			.property_type = FUEL_GAUGE_PROP_MAX,
		},
	};

	int ret = fixture->api->get_property(fixture->dev, props, ARRAY_SIZE(props));

	zassert_equal(props[0].status, -ENOTSUP, "Getting bad property %d has a good status.",
		      props[0].property_type);

	zassert_true(ret < 0);
}

ZTEST_F(sbs_gauge_new_api, test_get_some_props_failed_returns_failed_prop_count)
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

	int ret = fixture->api->get_property(fixture->dev, props, ARRAY_SIZE(props));

	zassert_equal(props[0].status, -ENOTSUP, "Getting bad property %d has a good status.",
		      props[0].property_type);

	zassert_equal(props[1].status, -ENOTSUP, "Getting bad property %d has a good status.",
		      props[1].property_type);

	zassert_ok(props[2].status, "Property %d getting %d has a bad status.", 2,
		   props[2].property_type);

	zassert_equal(ret, 2);
}

ZTEST_F(sbs_gauge_new_api, test_set_all_props_failed_returns_negative)
{
	struct fuel_gauge_set_property props[] = {
		{
			/* Invalid property */
			.property_type = FUEL_GAUGE_PROP_MAX,
		},
	};

	int ret = fixture->api->set_property(fixture->dev, props, ARRAY_SIZE(props));

	zassert_equal(props[0].status, -ENOTSUP, "Setting bad property %d has a good status.",
		      props[0].property_type);

	zassert_true(ret < 0);
}

ZTEST_F(sbs_gauge_new_api, test_set_some_props_failed_returns_failed_prop_count)
{
	struct fuel_gauge_set_property props[] = {
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
			.property_type = FUEL_GAUGE_SBS_MFR_ACCESS,
			/* Set Manufacturer's Access to arbitrary word */
			.value.sbs_mfr_access_word = 1,
		},

	};

	int ret = fixture->api->set_property(fixture->dev, props, ARRAY_SIZE(props));

	zassert_equal(props[0].status, -ENOTSUP, "Setting bad property %d has a good status.",
		      props[0].property_type);

	zassert_equal(props[1].status, -ENOTSUP, "Setting bad property %d has a good status.",
		      props[1].property_type);

	zassert_ok(props[2].status, "Property %d setting %d has a bad status.", 2,
		   props[2].property_type);

	zassert_equal(ret, 2);
}

ZTEST_F(sbs_gauge_new_api, test_set_prop_can_be_get)
{
	uint16_t word = BIT(15) | BIT(0);
	struct fuel_gauge_set_property set_prop = {
		/* Valid property */
		.property_type = FUEL_GAUGE_SBS_MFR_ACCESS,
		/* Set Manufacturer's Access to 16 bit word*/
		.value.sbs_mfr_access_word = word,
	};

	zassert_ok(fixture->api->set_property(fixture->dev, &set_prop, 1));
	zassert_ok(set_prop.status);

	struct fuel_gauge_get_property get_prop = {
		.property_type = FUEL_GAUGE_SBS_MFR_ACCESS,
	};

	zassert_ok(fixture->api->get_property(fixture->dev, &get_prop, 1));
	zassert_ok(get_prop.status);
	zassert_equal(get_prop.value.sbs_mfr_access_word, word);
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
		{
			.property_type = FUEL_GAUGE_SBS_MFR_ACCESS,
		},
	};

	int ret = fixture->api->get_property(fixture->dev, props, ARRAY_SIZE(props));

	for (int i = 0; i < ARRAY_SIZE(props); i++) {
		zassert_ok(props[i].status, "Property %d getting %d has a bad status.", i,
			   props[i].property_type);
	}

	zassert_ok(ret);
}

ZTEST_F(sbs_gauge_new_api, test_set_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	struct fuel_gauge_set_property props[] = {
		{
			.property_type = FUEL_GAUGE_SBS_MFR_ACCESS,
		},
	};

	int ret = fixture->api->set_property(fixture->dev, props, ARRAY_SIZE(props));

	for (int i = 0; i < ARRAY_SIZE(props); i++) {
		zassert_ok(props[i].status, "Property %d writing %d has a bad status.", i,
			   props[i].property_type);
	}

	zassert_ok(ret);
}

ZTEST_SUITE(sbs_gauge_new_api, NULL, sbs_gauge_new_api_setup, NULL, NULL, NULL);
