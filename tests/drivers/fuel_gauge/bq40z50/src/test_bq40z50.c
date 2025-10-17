/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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

struct bq40z50_fixture {
	const struct device *dev;
	const struct fuel_gauge_driver_api *api;
};

static void *bq40z50_setup(void)
{
	static ZTEST_DMEM struct bq40z50_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(ti_bq40z50);
	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

ZTEST_USER_F(bq40z50, test_get_some_props_failed_returns_bad_status)
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

ZTEST_USER_F(bq40z50, test_get_buffer_prop)
{
	int ret;

	{
		struct sbs_gauge_manufacturer_name mfg_name;

		ret = fuel_gauge_get_buffer_prop(fixture->dev, FUEL_GAUGE_MANUFACTURER_NAME,
						 &mfg_name, sizeof(mfg_name));
		zassert_ok(ret);
#if CONFIG_EMUL
		/* Only test for fixed values in emulation since the real device might be */
		/* reprogrammed and respond with different values */
		zassert_equal(sizeof("Texas Inst.") - 1, mfg_name.manufacturer_name_length);
		zassert_mem_equal(mfg_name.manufacturer_name, "Texas Inst.",
				  mfg_name.manufacturer_name_length,
				  "mfg_name.manufacturer_name='%s'", mfg_name.manufacturer_name);
#endif
	}
	{
		struct sbs_gauge_device_name dev_name;

		ret = fuel_gauge_get_buffer_prop(fixture->dev, FUEL_GAUGE_DEVICE_NAME, &dev_name,
						 sizeof(dev_name));
		zassert_ok(ret);
#if CONFIG_EMUL
		/* Only test for fixed values in emulation since the real device might be */
		/* reprogrammed and respond with different values */
		zassert_equal(sizeof("bq40z50") - 1, dev_name.device_name_length);
		zassert_mem_equal(dev_name.device_name, "bq40z50", dev_name.device_name_length);
#endif
	}
	{
		struct sbs_gauge_device_chemistry device_chemistry;

		ret = fuel_gauge_get_buffer_prop(fixture->dev, FUEL_GAUGE_DEVICE_CHEMISTRY,
						 &device_chemistry, sizeof(device_chemistry));
		zassert_ok(ret);
#if CONFIG_EMUL
		/* Only test for fixed values in emulation since the real device might be */
		/* reprogrammed and respond with different values */
		zassert_equal(sizeof("LION") - 1, device_chemistry.device_chemistry_length);
		zassert_mem_equal(device_chemistry.device_chemistry, "LION",
				  device_chemistry.device_chemistry_length);
#endif
	}
}

ZTEST_USER_F(bq40z50, test_get_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	fuel_gauge_prop_t props[] = {
		FUEL_GAUGE_AVG_CURRENT,
		FUEL_GAUGE_CURRENT,
		FUEL_GAUGE_CHARGE_CUTOFF,
		FUEL_GAUGE_CYCLE_COUNT,
		FUEL_GAUGE_FULL_CHARGE_CAPACITY,
		FUEL_GAUGE_REMAINING_CAPACITY,
		FUEL_GAUGE_RUNTIME_TO_EMPTY,
		FUEL_GAUGE_SBS_MFR_ACCESS,
		FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE,
		FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
		FUEL_GAUGE_TEMPERATURE,
		FUEL_GAUGE_VOLTAGE,
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
	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];

	zassert_ok(fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props)));
	/* Check properties for valid ranges */
#if CONFIG_EMUL
	zassert_equal(vals[0].avg_current, 1 * 1000);
	zassert_equal(vals[1].current, 1 * 1000);
	/* Not testing props[2]. This is the charger cutoff and has a boolean.*/
	zassert_equal(vals[3].cycle_count, 1);
	zassert_equal(vals[4].full_charge_capacity, 1 * 1000);
	zassert_equal(vals[5].remaining_capacity, 1 * 1000);
	zassert_equal(vals[6].runtime_to_empty, 65535);
	/* Not testing props[7]. This is the manufacturer access and has only status bits */
	zassert_equal(vals[8].absolute_state_of_charge, 100);
	zassert_equal(vals[9].relative_state_of_charge, 100);
	zassert_equal(vals[10].temperature, 2980);
	zassert_equal(vals[11].voltage, 1 * 1000);
	zassert_equal(vals[12].sbs_mode, 0);
	zassert_equal(vals[13].chg_current, 1 * 1000);
	zassert_equal(vals[14].chg_voltage, 1 * 1000);
	/* Not testing props[15]. This property is the status and only has only status bits */
	zassert_equal(vals[16].design_cap, 1);
	zassert_equal(vals[17].design_volt, 14400);
	zassert_equal(vals[18].sbs_at_rate, 0);
	zassert_equal(vals[19].sbs_at_rate_time_to_full, 65535);
	zassert_equal(vals[20].sbs_at_rate_time_to_empty, 65535);
	zassert_equal(vals[21].sbs_at_rate_ok, 0);
	zassert_equal(vals[22].sbs_remaining_capacity_alarm, 300);
	zassert_equal(vals[23].sbs_remaining_time_alarm, 10);
#else
	/* When having a real device, check for the valid ranges */
	zassert_between_inclusive(vals[0].avg_current, -32767 * 1000, 32768 * 1000);
	zassert_between_inclusive(vals[1].current, -32767 * 1000, 32768 * 1000);
	/* Not testing props[2]. This is the charger cutoff and has a boolean.*/
	zassert_between_inclusive(vals[3].cycle_count, 0, 65535);
	zassert_between_inclusive(vals[4].full_charge_capacity, 0, 65535 * 1000);
	zassert_between_inclusive(vals[5].remaining_capacity, 0, 65535 * 1000);
	zassert_between_inclusive(vals[6].runtime_to_empty, 0, 65535);
	/* Not testing props[7]. This is the manufacturer access and has only status bits */
	zassert_between_inclusive(vals[8].absolute_state_of_charge, 0, 100);
	zassert_between_inclusive(vals[9].relative_state_of_charge, 0, 100);
	zassert_between_inclusive(vals[10].temperature, 0, 65535);
	zassert_between_inclusive(vals[11].voltage, 0, 65535 * 1000);
	zassert_between_inclusive(vals[12].sbs_mode, 0, 65535 * 1000);
	zassert_between_inclusive(vals[13].chg_current, 0, 65535 * 1000);
	zassert_between_inclusive(vals[14].chg_voltage, 0, 65535 * 1000);
	/* Not testing props[15]. This property is the status and only has only status bits */
	zassert_between_inclusive(vals[16].design_cap, 0, 65535);
	zassert_between_inclusive(vals[17].design_volt, 0, 18000);
	zassert_between_inclusive(vals[18].sbs_at_rate, -32768, 32767);
	zassert_between_inclusive(vals[19].sbs_at_rate_time_to_full, 0, 65535);
	zassert_between_inclusive(vals[20].sbs_at_rate_time_to_empty, 0, 65535);
	/* Not testing props[21]. This is the sbs_at_rate_ok property and has a boolean.*/
	zassert_between_inclusive(vals[22].sbs_remaining_capacity_alarm, 0, 1000);
	zassert_between_inclusive(vals[23].sbs_remaining_time_alarm, 0, 30);
#endif
}

ZTEST_SUITE(bq40z50, NULL, bq40z50_setup, NULL, NULL, NULL);
