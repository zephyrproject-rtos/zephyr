/*
 * Copyright 2022 Google LLC
 * Copyright 2023 Microsoft Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_fuel_gauge.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include "test_sbs_gauge.h"

static void *sbs_gauge_new_api_setup(void)
{
	static ZTEST_DMEM struct sbs_gauge_new_api_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(sbs_sbs_gauge_new_api);
	fixture.sbs_fuel_gauge = EMUL_DT_GET(DT_NODELABEL(smartbattery0));

	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

ZTEST_USER_F(sbs_gauge_new_api, test_get_some_props_failed_returns_bad_status)
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

ZTEST_USER_F(sbs_gauge_new_api, test_set_all_props_failed_returns_err)
{
	fuel_gauge_prop_t prop_types[] = {
		/* Invalid property */
		FUEL_GAUGE_PROP_MAX,
	};
	union fuel_gauge_prop_val props[ARRAY_SIZE(prop_types)];

	int ret = fuel_gauge_set_props(fixture->dev, prop_types, props, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP);
}

ZTEST_USER_F(sbs_gauge_new_api, test_set_some_props_failed_returns_err)
{
	fuel_gauge_prop_t prop_types[] = {
		/* First invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Second invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Valid property */
		FUEL_GAUGE_SBS_MFR_ACCESS,
		/* Set Manufacturer's Access to arbitrary word */

	};

	union fuel_gauge_prop_val props[] = {
		/* First invalid property */
		{0},
		/* Second invalid property */
		{0},
		/* Valid property */
		/* Set Manufacturer's Access to arbitrary word */
		{.sbs_mfr_access_word = 1},
	};

	int ret = fuel_gauge_set_props(fixture->dev, prop_types, props, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP);
}

ZTEST_USER_F(sbs_gauge_new_api, test_set_prop_can_be_get)
{
	uint16_t word = BIT(15) | BIT(0);

	fuel_gauge_prop_t prop_types[] = {
		FUEL_GAUGE_SBS_MFR_ACCESS,
		FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM,
		FUEL_GAUGE_SBS_REMAINING_TIME_ALARM,
		FUEL_GAUGE_SBS_MODE,
		FUEL_GAUGE_SBS_ATRATE,
	};

	union fuel_gauge_prop_val set_props[] = {
		{
			.sbs_mfr_access_word = word,
		},
		{
			.sbs_remaining_capacity_alarm = word,
		},
		{
			.sbs_remaining_time_alarm = word,
		},
		{
			.sbs_mode = word,
		},
		{
			.sbs_at_rate = (int16_t)word,
		},
	};

	union fuel_gauge_prop_val get_props[ARRAY_SIZE(prop_types)];

	zassert_ok(
		fuel_gauge_set_props(fixture->dev, prop_types, set_props, ARRAY_SIZE(set_props)));

	zassert_ok(
		fuel_gauge_get_props(fixture->dev, prop_types, get_props, ARRAY_SIZE(get_props)));

	zassert_equal(get_props[0].sbs_mfr_access_word, word);
	zassert_equal(get_props[1].sbs_remaining_capacity_alarm, word);
	zassert_equal(get_props[2].sbs_remaining_time_alarm, word);
	zassert_equal(get_props[3].sbs_mode, word);
	zassert_equal(get_props[4].sbs_at_rate, (int16_t)word);
}

ZTEST_USER_F(sbs_gauge_new_api, test_get_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	fuel_gauge_prop_t prop_types[] = {
		FUEL_GAUGE_VOLTAGE,
		FUEL_GAUGE_CURRENT,
		FUEL_GAUGE_AVG_CURRENT,
		FUEL_GAUGE_TEMPERATURE,
		FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE,
		FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
		FUEL_GAUGE_RUNTIME_TO_FULL,
		FUEL_GAUGE_RUNTIME_TO_EMPTY,
		FUEL_GAUGE_REMAINING_CAPACITY,
		FUEL_GAUGE_FULL_CHARGE_CAPACITY,
		FUEL_GAUGE_CYCLE_COUNT,
		FUEL_GAUGE_SBS_MFR_ACCESS,
		FUEL_GAUGE_SBS_MODE,
		FUEL_GAUGE_CHARGE_CURRENT,
		FUEL_GAUGE_CHARGE_VOLTAGE,
		FUEL_GAUGE_STATUS,
		FUEL_GAUGE_DESIGN_CAPACITY,
		FUEL_GAUGE_DESIGN_VOLTAGE,
		FUEL_GAUGE_SBS_ATRATE,
		FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL,
		FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY,
		FUEL_GAUGE_SBS_ATRATE_OK,
		FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM,
		FUEL_GAUGE_SBS_REMAINING_TIME_ALARM,
	};

	union fuel_gauge_prop_val props[ARRAY_SIZE(prop_types)];

	zassert_ok(fuel_gauge_get_props(fixture->dev, prop_types, props, ARRAY_SIZE(props)));
}

ZTEST_USER_F(sbs_gauge_new_api, test_set_props__returns_ok)
{
	fuel_gauge_prop_t prop_types[] = {
		FUEL_GAUGE_SBS_MFR_ACCESS,
		FUEL_GAUGE_SBS_MODE,
		FUEL_GAUGE_SBS_ATRATE,
		FUEL_GAUGE_SBS_REMAINING_TIME_ALARM,
		FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM,

	};
	union fuel_gauge_prop_val props[ARRAY_SIZE(prop_types)];

	zassert_ok(fuel_gauge_set_props(fixture->dev, prop_types, props, ARRAY_SIZE(props)));
}

ZTEST_USER_F(sbs_gauge_new_api, test_get_buffer_props__returns_ok)
{
	/* Validate what properties are supported by the driver */
	struct sbs_gauge_manufacturer_name mfg_name;
	struct sbs_gauge_device_name dev_name;
	struct sbs_gauge_device_chemistry chem;

	zassert_ok(fuel_gauge_get_buffer_prop(fixture->dev, FUEL_GAUGE_MANUFACTURER_NAME, &mfg_name,
					      sizeof(mfg_name)));

	zassert_ok(fuel_gauge_get_buffer_prop(fixture->dev, FUEL_GAUGE_DEVICE_NAME, &dev_name,
					      sizeof(dev_name)));

	zassert_ok(fuel_gauge_get_buffer_prop(fixture->dev, FUEL_GAUGE_DEVICE_CHEMISTRY, &chem,
					      sizeof(chem)));
}

ZTEST_USER_F(sbs_gauge_new_api, test_charging_5v_3a)
{
	uint32_t expected_uV = 5000 * 1000;
	int32_t expected_uA = 3000 * 1000;

	union fuel_gauge_prop_val voltage;
	union fuel_gauge_prop_val current;

	zassume_ok(emul_fuel_gauge_set_battery_charging(fixture->sbs_fuel_gauge, expected_uV,
							expected_uA));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_VOLTAGE, &voltage));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CURRENT, &current));

	zassert_equal(voltage.voltage, expected_uV, "Got %d instead of %d", voltage, expected_uV);
	zassert_equal(current.current, expected_uA, "Got %d instead of %d", current, expected_uA);
}

ZTEST_USER_F(sbs_gauge_new_api, test_charging_5v_neg_1a)
{
	uint32_t expected_uV = 5000 * 1000;
	int32_t expected_uA = -1000 * 1000;

	union fuel_gauge_prop_val voltage;
	union fuel_gauge_prop_val current;

	zassume_ok(emul_fuel_gauge_set_battery_charging(fixture->sbs_fuel_gauge, expected_uV,
							expected_uA));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_VOLTAGE, &voltage));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CURRENT, &current));

	zassert_equal(voltage.voltage, expected_uV, "Got %d instead of %d", voltage, expected_uV);
	zassert_equal(current.current, expected_uA, "Got %d instead of %d", current, expected_uA);
}

ZTEST_USER_F(sbs_gauge_new_api, test_set_get_single_prop)
{
	uint16_t test_value = 0x1001;

	union fuel_gauge_prop_val mfr_acc_set = {
		.sbs_mfr_access_word = test_value,
	};
	union fuel_gauge_prop_val mfr_acc_get;

	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_SBS_MFR_ACCESS, mfr_acc_set));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_SBS_MFR_ACCESS, &mfr_acc_get));
	zassert_equal(mfr_acc_get.sbs_mfr_access_word, test_value);
}

ZTEST_SUITE(sbs_gauge_new_api, NULL, sbs_gauge_new_api_setup, NULL, NULL, NULL);
