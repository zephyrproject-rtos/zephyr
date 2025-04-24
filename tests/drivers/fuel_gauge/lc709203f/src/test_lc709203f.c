/*
 * Copyright (c) 2025 Philipp Steiner <philipp.steiner1987@gmail.com>
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

struct lc709203f_fixture {
	const struct device *dev;
	const struct fuel_gauge_driver_api *api;
};

static void *lc709203f_setup(void)
{
	static ZTEST_DMEM struct lc709203f_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(onnn_lc709203f);
	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

ZTEST_USER_F(lc709203f, test_get_some_props_failed__returns_bad_status)
{
	fuel_gauge_prop_t props[] = {
		/* First invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Second invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Valid property */
		FUEL_GAUGE_VOLTAGE,
	};
	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];

	int ret = fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP, "Getting bad property has a good status.");
}

ZTEST_USER_F(lc709203f, test_set_all_props_failed__returns_err)
{
	fuel_gauge_prop_t prop_types[] = {
		/* Invalid property */
		FUEL_GAUGE_PROP_MAX,
	};
	union fuel_gauge_prop_val props[ARRAY_SIZE(prop_types)] = {0};

	int ret = fuel_gauge_set_props(fixture->dev, prop_types, props, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP);
}

ZTEST_USER_F(lc709203f, test_set_some_props_failed__returns_err)
{
	fuel_gauge_prop_t prop_types[] = {
		/* First invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Second invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Valid property */
		FUEL_GAUGE_STATE_OF_CHARGE_ALARM,
		/* Set Manufacturer's Access to arbitrary word */

	};

	union fuel_gauge_prop_val props[] = {
		/* First invalid property */
		{0},
		/* Second invalid property */
		{0},
		/* Valid property */
		/* Sets state of charge threshold to generate Alarm signal*/
		{.state_of_charge_alarm = 10},
	};

	int ret = fuel_gauge_set_props(fixture->dev, prop_types, props, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP);
}

ZTEST_USER_F(lc709203f, test_set_prop_can_be_get)
{
	uint16_t sbs_mode = 0x0002;
	uint16_t current_direction = 0x0001;
	uint8_t state_of_charge_alarm = 20;
	uint32_t low_voltage_alarm = 3000 * 1000;

	fuel_gauge_prop_t prop_types[] = {
		FUEL_GAUGE_SBS_MODE,
		FUEL_GAUGE_CURRENT_DIRECTION,
		FUEL_GAUGE_STATE_OF_CHARGE_ALARM,
		FUEL_GAUGE_LOW_VOLTAGE_ALARM,
	};

	union fuel_gauge_prop_val set_props[] = {
		{
			.sbs_mode = sbs_mode,
		},
		{
			.current_direction = current_direction,
		},
		{
			.state_of_charge_alarm = state_of_charge_alarm,
		},
		{
			.low_voltage_alarm = low_voltage_alarm,
		},
	};

	union fuel_gauge_prop_val get_props[ARRAY_SIZE(prop_types)];

	zassert_ok(
		fuel_gauge_set_props(fixture->dev, prop_types, set_props, ARRAY_SIZE(set_props)));

	zassert_ok(
		fuel_gauge_get_props(fixture->dev, prop_types, get_props, ARRAY_SIZE(get_props)));

	zassert_equal(get_props[0].sbs_mode, sbs_mode);
	zassert_equal(get_props[1].current_direction, current_direction);
	zassert_equal(get_props[2].state_of_charge_alarm, state_of_charge_alarm);
	zassert_equal(get_props[3].low_voltage_alarm, low_voltage_alarm);
}

ZTEST_USER_F(lc709203f, test_get_props__returns_ok)
{
	fuel_gauge_prop_t props[] = {
		FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
		FUEL_GAUGE_TEMPERATURE,
		FUEL_GAUGE_VOLTAGE,
		FUEL_GAUGE_SBS_MODE,
		FUEL_GAUGE_DESIGN_CAPACITY,
		FUEL_GAUGE_CURRENT_DIRECTION,
		FUEL_GAUGE_STATE_OF_CHARGE_ALARM,
		FUEL_GAUGE_LOW_VOLTAGE_ALARM,
	};
	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];

	int ret = fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props));

#if CONFIG_EMUL
	zassert_equal(vals[0].relative_state_of_charge, 50);
	zassert_equal(vals[1].temperature, 0x0BA6);
	zassert_equal(vals[2].voltage, 3700 * 1000);
	zassert_equal(vals[3].sbs_mode, 0x0001);
	zassert_equal(vals[4].design_cap, 500);
	zassert_true(((vals[5].current_direction == 0x0000) ||
		      (vals[5].current_direction == 0x0001) ||
		      (vals[5].current_direction == 0xFFFF)));
	zassert_equal(vals[6].state_of_charge_alarm, 0x0008);
	zassert_equal(vals[7].low_voltage_alarm, 0x0000);
#else
	zassert_between_inclusive(vals[0].relative_state_of_charge, 0, 100);
	zassert_between_inclusive(vals[1].temperature, 0x09E4, 0x0D04);
	zassert_between_inclusive(vals[2].voltage, 0, 0xFFFF * 1000);
	zassert_between_inclusive(vals[3].sbs_mode, 0x0001, 0x0002);
	zassert_true(((vals[4].design_cap == 100) || (vals[4].design_cap == 200) ||
		      (vals[4].design_cap == 500) || (vals[4].design_cap == 1000) ||
		      (vals[4].design_cap == 3000)));
	zassert_true(((vals[5].current_direction == 0x0000) ||
		      (vals[5].current_direction == 0x0001) ||
		      (vals[5].current_direction == 0xFFFF)));
	zassert_between_inclusive(vals[6].state_of_charge_alarm, 0, 100);
	zassert_between_inclusive(vals[7].low_voltage_alarm, 0, 0xFFFF * 1000);
#endif

	zassert_equal(ret, 0, "Getting bad property has a good status.");
}

ZTEST_USER_F(lc709203f, test_set_get_single_prop)
{
	uint8_t test_value = 5;

	union fuel_gauge_prop_val state_of_charge_alarm_set = {
		.state_of_charge_alarm = test_value,
	};
	union fuel_gauge_prop_val state_of_charge_alarm_get;

	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_STATE_OF_CHARGE_ALARM,
				       state_of_charge_alarm_set));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_STATE_OF_CHARGE_ALARM,
				       &state_of_charge_alarm_get));
	zassert_equal(state_of_charge_alarm_get.state_of_charge_alarm, test_value);
}

ZTEST_SUITE(lc709203f, NULL, lc709203f_setup, NULL, NULL, NULL);
