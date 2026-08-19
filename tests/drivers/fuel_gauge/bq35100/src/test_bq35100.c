/*
 * Copyright (c) 2025 Orgatex GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/fuel_gauge/bq35100_user.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

/* Readings reported by emul_bq35100.c */
#define EXPECTED_VOLTAGE_UV   3600000
#define EXPECTED_CURRENT_UA   (-25000)
#define EXPECTED_DESIGN_CAP   1200
#define EXPECTED_REMAINING    900000
#define EXPECTED_SOC          75

static const struct gpio_dt_spec ge_pin =
	GPIO_DT_SPEC_GET(DT_NODELABEL(bq35100), supply_gpios);

struct bq35100_fixture {
	const struct device *dev;
};

static void *bq35100_setup(void)
{
	static ZTEST_DMEM struct bq35100_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(ti_bq35100);
	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

ZTEST_USER_F(bq35100, test_get_some_props_failed_returns_bad_status)
{
	fuel_gauge_prop_t props[] = {
		/* First invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Second invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Valid property */
		FUEL_GAUGE_VOLTAGE_UV,
	};
	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];

	int ret = fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP, "Getting bad property has a good status.");
}

ZTEST_USER_F(bq35100, test_get_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	fuel_gauge_prop_t props[] = {
		FUEL_GAUGE_VOLTAGE_UV,
		FUEL_GAUGE_CURRENT_UA,
		FUEL_GAUGE_DESIGN_CAPACITY,
		FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT,
		FUEL_GAUGE_REMAINING_CAPACITY_UAH,
	};
	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];

	zassert_ok(fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props)));

	zassert_equal(vals[0].voltage_uv, EXPECTED_VOLTAGE_UV);
	zassert_equal(vals[1].current_ua, EXPECTED_CURRENT_UA);
	zassert_equal(vals[2].design_cap, EXPECTED_DESIGN_CAP);
	zassert_equal(vals[3].absolute_state_of_charge_pct, EXPECTED_SOC);
	zassert_equal(vals[4].remaining_capacity_uah, EXPECTED_REMAINING);
}

ZTEST_F(bq35100, test_init_drives_ge_pin_high)
{
	ARG_UNUSED(fixture);

	zassert_true(gpio_is_ready_dt(&ge_pin), "GE pin not bound");
	zassert_equal(gpio_emul_output_get_dt(&ge_pin), 1,
		      "Gauge is not enabled after initialization");
}

ZTEST_F(bq35100, test_suspend_releases_ge_pin)
{
	zassert_ok(pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_SUSPEND));
	zassert_equal(gpio_emul_output_get_dt(&ge_pin), 0,
		      "Gauge stays enabled while suspended");

	zassert_ok(pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_RESUME));
	zassert_equal(gpio_emul_output_get_dt(&ge_pin), 1,
		      "Gauge is not enabled again after resume");
}

ZTEST_SUITE(bq35100, NULL, bq35100_setup, NULL, NULL, NULL);
