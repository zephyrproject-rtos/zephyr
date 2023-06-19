/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
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

struct bq27z746_fixture {
	const struct device *dev;
	const struct fuel_gauge_driver_api *api;
};

static void *bq27z746_setup(void)
{
	static ZTEST_DMEM struct bq27z746_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(ti_bq27z746);
	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

ZTEST_USER_F(bq27z746, test_get_all_props_failed_returns_negative)
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

ZTEST_USER_F(bq27z746, test_get_some_props_failed_returns_failed_prop_count)
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

ZTEST_USER_F(bq27z746, test_get_buffer_prop)
{
	struct fuel_gauge_get_buffer_property prop;
	int ret;

	{
		struct sbs_gauge_manufacturer_name mfg_name;

		prop.property_type = FUEL_GAUGE_MANUFACTURER_NAME;
		ret = fuel_gauge_get_buffer_prop(fixture->dev, &prop, &mfg_name, sizeof(mfg_name));
		zassert_ok(ret);
		zassert_ok(prop.status, "Property %d has a bad status.", prop.property_type);
#if CONFIG_EMUL
		/* Only test for fixed values in emulation since the real device might be */
		/* reprogrammed and respond with different values */
		zassert_equal(sizeof("Texas Instruments") - 1, mfg_name.manufacturer_name_length);
		zassert_mem_equal(mfg_name.manufacturer_name, "Texas Instruments",
				  mfg_name.manufacturer_name_length,
				  "mfg_name.manufacturer_name='%s'", mfg_name.manufacturer_name);
#endif
	}
	{
		struct sbs_gauge_device_name dev_name;

		prop.property_type = FUEL_GAUGE_DEVICE_NAME;
		ret = fuel_gauge_get_buffer_prop(fixture->dev, &prop, &dev_name, sizeof(dev_name));
		zassert_ok(ret);
		zassert_ok(prop.status, "Property %d has a bad status.", prop.property_type);
#if CONFIG_EMUL
		/* Only test for fixed values in emulation since the real device might be */
		/* reprogrammed and respond with different values */
		zassert_equal(sizeof("BQ27Z746") - 1, dev_name.device_name_length);
		zassert_mem_equal(dev_name.device_name, "BQ27Z746", dev_name.device_name_length);
#endif
	}
	{
		struct sbs_gauge_device_chemistry device_chemistry;

		prop.property_type = FUEL_GAUGE_DEVICE_CHEMISTRY;
		ret = fuel_gauge_get_buffer_prop(fixture->dev, &prop, &device_chemistry,
						 sizeof(device_chemistry));
		zassert_ok(ret);
		zassert_ok(prop.status, "Property %d has a bad status.", prop.property_type);
#if CONFIG_EMUL
		/* Only test for fixed values in emulation since the real device might be */
		/* reprogrammed and respond with different values */
		zassert_equal(sizeof("LION") - 1, device_chemistry.device_chemistry_length);
		zassert_mem_equal(device_chemistry.device_chemistry, "LION",
				  device_chemistry.device_chemistry_length);
#endif
	}
}

ZTEST_USER_F(bq27z746, test_get_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	struct fuel_gauge_get_property props[] = {
		{
			.property_type = FUEL_GAUGE_AVG_CURRENT,
		},
		{
			.property_type = FUEL_GAUGE_CYCLE_COUNT,
		},
		{
			.property_type = FUEL_GAUGE_CURRENT,
		},
		{
			.property_type = FUEL_GAUGE_FULL_CHARGE_CAPACITY,
		},
		{
			.property_type = FUEL_GAUGE_REMAINING_CAPACITY,
		},
		{
			.property_type = FUEL_GAUGE_RUNTIME_TO_EMPTY,
		},
		{
			.property_type = FUEL_GAUGE_RUNTIME_TO_FULL,
		},
		{
			.property_type = FUEL_GAUGE_SBS_MFR_ACCESS,
		},
		{
			.property_type = FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
		},
		{
			.property_type = FUEL_GAUGE_TEMPERATURE,
		},
		{
			.property_type = FUEL_GAUGE_VOLTAGE,
		},
		{
			.property_type = FUEL_GAUGE_SBS_ATRATE,
		},
		{
			.property_type = FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY,
		},
		{
			.property_type = FUEL_GAUGE_CHARGE_VOLTAGE,
		},
		{
			.property_type = FUEL_GAUGE_CHARGE_CURRENT,
		},
		{
			.property_type = FUEL_GAUGE_STATUS,
		},
		{
			.property_type = FUEL_GAUGE_DESIGN_CAPACITY,
		},
	};

	int ret = fuel_gauge_get_prop(fixture->dev, props, ARRAY_SIZE(props));

	/* All props shall have a good status */
	for (int i = 0; i < ARRAY_SIZE(props); i++) {
		zassert_ok(props[i].status, "Property %d getting %d has a bad status.", i,
			   props[i].property_type);
	}

	/* Check properties for valid ranges */
#if CONFIG_EMUL
	/* When emulating, check for the fixed values coming from the emulator */
	zassert_equal(props[0].value.avg_current, -2000);
	zassert_equal(props[1].value.cycle_count, 100);
	zassert_equal(props[2].value.current, -2000);
	zassert_equal(props[3].value.full_charge_capacity, 1000);
	zassert_equal(props[4].value.remaining_capacity, 1000);
	zassert_equal(props[5].value.runtime_to_empty, 1);
	zassert_equal(props[6].value.runtime_to_full, 1);
	zassert_equal(props[7].value.sbs_mfr_access_word, 1);
	zassert_equal(props[8].value.relative_state_of_charge, 1);
	zassert_equal(props[9].value.temperature, 1);
	zassert_equal(props[10].value.voltage, 1000);
	zassert_equal(props[11].value.sbs_at_rate, -2);
	zassert_equal(props[12].value.sbs_at_rate_time_to_empty, 1);
	zassert_equal(props[13].value.chg_voltage, 1);
	zassert_equal(props[14].value.chg_current, 1);
	zassert_equal(props[15].value.fg_status, 1);
	zassert_equal(props[16].value.design_cap, 1);
#else
	/* When having a real device, check for the valid ranges */
	zassert_between_inclusive(props[0].value.avg_current, -32768 * 1000, 32767 * 1000);
	zassert_between_inclusive(props[1].value.cycle_count, 0, 6553500);
	zassert_between_inclusive(props[2].value.current, -32768 * 1000, 32767 * 1000);
	zassert_between_inclusive(props[3].value.full_charge_capacity, 0, 32767 * 1000);
	zassert_between_inclusive(props[4].value.remaining_capacity, 0, 32767 * 1000);
	zassert_between_inclusive(props[5].value.runtime_to_empty, 0, 65535);
	zassert_between_inclusive(props[6].value.runtime_to_full, 0, 65535);
	/* Not testing props[7]. This is the manufacturer access and has only status bits */
	zassert_between_inclusive(props[8].value.relative_state_of_charge, 0, 100);
	zassert_between_inclusive(props[9].value.temperature, 0, 32767);
	zassert_between_inclusive(props[10].value.voltage, 0, 32767 * 1000);
	zassert_between_inclusive(props[11].value.sbs_at_rate, -32768, 32767);
	zassert_between_inclusive(props[12].value.sbs_at_rate_time_to_empty, 0, 65535);
	zassert_between_inclusive(props[13].value.chg_voltage, 0, 32767);
	zassert_between_inclusive(props[14].value.chg_current, 0, 32767);
	/* Not testing props[15]. This property is the status and only has only status bits */
	zassert_between_inclusive(props[16].value.design_cap, 0, 32767);
#endif

	zassert_ok(ret);
}

ZTEST_SUITE(bq27z746, NULL, bq27z746_setup, NULL, NULL, NULL);
