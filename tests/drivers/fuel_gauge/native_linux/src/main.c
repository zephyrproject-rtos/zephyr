/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/ztest.h>

/* The attribute files of the fake power supply are in power_supply/BAT0. */
static const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(fuel_gauge0));

static void *native_linux_fuel_gauge_setup(void)
{
	zassert_true(device_is_ready(dev), "fuel gauge not ready");

	return NULL;
}

ZTEST(native_linux_fuel_gauge, test_get_props)
{
	const fuel_gauge_prop_t props[] = {
		FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT,
		FUEL_GAUGE_VOLTAGE_UV,
		FUEL_GAUGE_REMAINING_CAPACITY_UAH,
		FUEL_GAUGE_DESIGN_CAPACITY,
	};
	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];
	int ret;

	ret = fuel_gauge_get_props(dev, props, vals, ARRAY_SIZE(props));
	zassert_ok(ret, "get_props failed: %d", ret);

	zassert_equal(vals[0].relative_state_of_charge_pct, 87);
	zassert_equal(vals[1].voltage_uv, 12345000);
	zassert_equal(vals[2].remaining_capacity_uah, 3456000);
	/* charge_full_design is 5100000 uAh, the property is in mAh */
	zassert_equal(vals[3].design_cap, 5100);
}

ZTEST(native_linux_fuel_gauge, test_missing_attribute_is_unsupported)
{
	union fuel_gauge_prop_val val;
	int ret;

	ret = fuel_gauge_get_prop(dev, FUEL_GAUGE_CYCLE_COUNT, &val);
	zassert_equal(ret, -ENOTSUP, "missing attribute returned %d", ret);
}

ZTEST(native_linux_fuel_gauge, test_non_numeric_value)
{
	union fuel_gauge_prop_val val;
	int ret;

	ret = fuel_gauge_get_prop(dev, FUEL_GAUGE_CURRENT_UA, &val);
	zassert_equal(ret, -EINVAL, "non-numeric attribute returned %d", ret);
}

ZTEST(native_linux_fuel_gauge, test_value_out_of_range)
{
	union fuel_gauge_prop_val val;
	int ret;

	ret = fuel_gauge_get_prop(dev, FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH, &val);
	zassert_equal(ret, -ERANGE, "out of range attribute returned %d", ret);
}

ZTEST(native_linux_fuel_gauge, test_names)
{
	struct sbs_gauge_manufacturer_name mfg;
	struct sbs_gauge_device_name model;
	int ret;

	ret = fuel_gauge_get_buffer_prop(dev, FUEL_GAUGE_MANUFACTURER_NAME, &mfg, sizeof(mfg));
	zassert_ok(ret, "manufacturer name failed: %d", ret);
	/* A name as long as the field uses every byte of it */
	zassert_equal(mfg.manufacturer_name_length, SBS_GAUGE_MANUFACTURER_NAME_MAX_SIZE);
	zassert_mem_equal(mfg.manufacturer_name, "ZephyrProjectBattery",
			  SBS_GAUGE_MANUFACTURER_NAME_MAX_SIZE);

	ret = fuel_gauge_get_buffer_prop(dev, FUEL_GAUGE_DEVICE_NAME, &model, sizeof(model));
	zassert_ok(ret, "device name failed: %d", ret);
	zassert_equal(model.device_name_length, strlen("Fake"));
	zassert_str_equal(model.device_name, "Fake");
}

ZTEST_SUITE(native_linux_fuel_gauge, NULL, native_linux_fuel_gauge_setup, NULL, NULL, NULL);
