/*
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

/* Must track the readings in drivers/fuel_gauge/bq4xz50/emul_bq4xz50.c. */
#define EXP_TEMPERATURE_DK    2980
#define EXP_VOLTAGE_UV        (16800 * 1000)
#define EXP_CURRENT_UA        (-500 * 1000)
#define EXP_AVG_CURRENT_UA    (-450 * 1000)
#define EXP_REMAINING_CAP_UAH (3500 * 1000)
#define EXP_FULL_CHARGE_UAH   (4000 * 1000)
#define EXP_CHARGING_CURR_UA  (2000 * 1000)
#define EXP_CHARGING_VOLT_UV  (16800 * 1000)
#define EXP_DESIGN_CAP        4000
#define EXP_DESIGN_VOLT_MV    14400
#define EXP_BATTERY_STATUS    0x00C0
#define EXP_OPERATION_STATUS  0x00000203
#define EXP_CYCLE_COUNT       67
#define EXP_REL_CHARGE_PCT    87
#define EXP_ABS_CHARGE_PCT    85
#define EXP_STATE_OF_HEALTH   95
#define EXP_RUNTIME_EMPTY_MIN 420
#define EXP_TIME_UNKNOWN      0xFFFF
#define EXP_BATTERY_MODE      0
#define EXP_AT_RATE           0
#define EXP_AT_RATE_OK        0
#define EXP_CAP_ALARM         300
#define EXP_TIME_ALARM        10

/* BatteryMode CAPACITY_MODE: capacity registers switch from mAh to 10 mWh. */
#define BATTERY_MODE_CAPM BIT(15)

struct bq41z50_fixture {
	const struct device *dev;
};

static void *bq41z50_setup(void)
{
	static ZTEST_DMEM struct bq41z50_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(ti_bq41z50);
	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

ZTEST_USER_F(bq41z50, test_get_some_props_failed_returns_bad_status)
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

ZTEST_USER_F(bq41z50, test_set_prop_unsupported_returns_notsup)
{
	union fuel_gauge_prop_val val = {0};

	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_PROP_MAX, val), -ENOTSUP);
}

/*
 * DeviceName is the one reading the emulator varies per compatible, so this also proves the
 * bq41z50 instance is bound to its own emulator rather than the bq40z50 one.
 */
ZTEST_USER_F(bq41z50, test_get_buffer_prop)
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
		/* The payload is length-delimited and not NULL terminated. */
		zassert_mem_equal(mfg_name.manufacturer_name, "Texas Inst.",
				  mfg_name.manufacturer_name_length,
				  "mfg_name.manufacturer_name='%.*s'",
				  mfg_name.manufacturer_name_length, mfg_name.manufacturer_name);
#endif
	}
	{
		struct sbs_gauge_device_name dev_name;

		ret = fuel_gauge_get_buffer_prop(fixture->dev, FUEL_GAUGE_DEVICE_NAME, &dev_name,
						 sizeof(dev_name));
		zassert_ok(ret);
#if CONFIG_EMUL
		zassert_equal(sizeof("bq41z50") - 1, dev_name.device_name_length);
		zassert_mem_equal(dev_name.device_name, "bq41z50", dev_name.device_name_length);
#endif
	}
	{
		struct sbs_gauge_device_chemistry device_chemistry;

		ret = fuel_gauge_get_buffer_prop(fixture->dev, FUEL_GAUGE_DEVICE_CHEMISTRY,
						 &device_chemistry, sizeof(device_chemistry));
		zassert_ok(ret);
#if CONFIG_EMUL
		zassert_equal(sizeof("LION") - 1, device_chemistry.device_chemistry_length);
		zassert_mem_equal(device_chemistry.device_chemistry, "LION",
				  device_chemistry.device_chemistry_length);
#endif
	}
}

ZTEST_USER_F(bq41z50, test_get_buffer_prop_bad_size_returns_einval)
{
	struct sbs_gauge_device_name dev_name;

	zassert_equal(fuel_gauge_get_buffer_prop(fixture->dev, FUEL_GAUGE_DEVICE_NAME, &dev_name,
						 sizeof(dev_name) - 1),
		      -EINVAL);
}

ZTEST_USER_F(bq41z50, test_get_props__returns_ok)
{
	/* Validate what props are supported by the driver */

	fuel_gauge_prop_t props[] = {
		FUEL_GAUGE_AVG_CURRENT_UA,
		FUEL_GAUGE_CURRENT_UA,
		FUEL_GAUGE_CYCLE_COUNT,
		FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH,
		FUEL_GAUGE_REMAINING_CAPACITY_UAH,
		FUEL_GAUGE_RUNTIME_TO_EMPTY_MINS,
		FUEL_GAUGE_RUNTIME_TO_FULL_MINS,
		FUEL_GAUGE_SBS_MFR_ACCESS,
		FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT,
		FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT,
		FUEL_GAUGE_TEMPERATURE_DK,
		FUEL_GAUGE_VOLTAGE_UV,
		FUEL_GAUGE_SBS_MODE,
		FUEL_GAUGE_CHARGE_CURRENT_UA,
		FUEL_GAUGE_CHARGE_VOLTAGE_UV,
		FUEL_GAUGE_STATUS,
		FUEL_GAUGE_DESIGN_CAPACITY,
		FUEL_GAUGE_DESIGN_VOLTAGE_MV,
		FUEL_GAUGE_SBS_ATRATE,
		FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL_MINS,
		FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY_MINS,
		FUEL_GAUGE_SBS_ATRATE_OK,
		FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM,
		FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS,
		FUEL_GAUGE_STATE_OF_HEALTH,
	};

	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];

	zassert_ok(fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props)));
#if CONFIG_EMUL
	zassert_equal(vals[0].avg_current_ua, EXP_AVG_CURRENT_UA);
	zassert_equal(vals[1].current_ua, EXP_CURRENT_UA);
	zassert_equal(vals[2].cycle_count, EXP_CYCLE_COUNT);
	zassert_equal(vals[3].full_charge_capacity_uah, EXP_FULL_CHARGE_UAH);
	zassert_equal(vals[4].remaining_capacity_uah, EXP_REMAINING_CAP_UAH);
	zassert_equal(vals[5].runtime_to_empty_mins, EXP_RUNTIME_EMPTY_MIN);
	zassert_equal(vals[6].runtime_to_full_mins, EXP_TIME_UNKNOWN);
	/* Not testing props[7]. ManufacturerAccess reflects the last subcommand issued. */
	zassert_equal(vals[8].absolute_state_of_charge_pct, EXP_ABS_CHARGE_PCT);
	zassert_equal(vals[9].relative_state_of_charge_pct, EXP_REL_CHARGE_PCT);
	zassert_equal(vals[10].temperature_dk, EXP_TEMPERATURE_DK);
	zassert_equal(vals[11].voltage_uv, EXP_VOLTAGE_UV);
	zassert_equal(vals[12].sbs_mode, EXP_BATTERY_MODE);
	zassert_equal(vals[13].chg_current_ua, EXP_CHARGING_CURR_UA);
	zassert_equal(vals[14].chg_voltage_uv, EXP_CHARGING_VOLT_UV);
	zassert_equal(vals[15].fg_status, EXP_BATTERY_STATUS);
	zassert_equal(vals[16].design_cap, EXP_DESIGN_CAP);
	zassert_equal(vals[17].design_volt_mv, EXP_DESIGN_VOLT_MV);
	zassert_equal(vals[18].sbs_at_rate, EXP_AT_RATE);
	zassert_equal(vals[19].sbs_at_rate_time_to_full_mins, EXP_TIME_UNKNOWN);
	zassert_equal(vals[20].sbs_at_rate_time_to_empty_mins, EXP_TIME_UNKNOWN);
	zassert_equal(vals[21].sbs_at_rate_ok, EXP_AT_RATE_OK);
	zassert_equal(vals[22].sbs_remaining_capacity_alarm, EXP_CAP_ALARM);
	zassert_equal(vals[23].sbs_remaining_time_alarm_mins, EXP_TIME_ALARM);
	zassert_equal(vals[24].state_of_health, EXP_STATE_OF_HEALTH);
#else
	/* When having a real device, check for the valid ranges */
	zassert_between_inclusive(vals[0].avg_current_ua, -32768 * 1000, 32767 * 1000);
	zassert_between_inclusive(vals[1].current_ua, -32768 * 1000, 32767 * 1000);
	zassert_between_inclusive(vals[2].cycle_count, 0, 65535);
	zassert_between_inclusive(vals[3].full_charge_capacity_uah, 0, 65535 * 1000);
	zassert_between_inclusive(vals[4].remaining_capacity_uah, 0, 65535 * 1000);
	zassert_between_inclusive(vals[5].runtime_to_empty_mins, 0, 65535);
	zassert_between_inclusive(vals[6].runtime_to_full_mins, 0, 65535);
	/* Not testing props[7]. Manufacturer access only carries status bits. */
	zassert_between_inclusive(vals[8].absolute_state_of_charge_pct, 0, 100);
	zassert_between_inclusive(vals[9].relative_state_of_charge_pct, 0, 100);
	zassert_between_inclusive(vals[10].temperature_dk, 0, 65535);
	zassert_between_inclusive(vals[11].voltage_uv, 0, 65535 * 1000);
	/* Not testing props[12]. Battery mode only carries flag bits. */
	zassert_between_inclusive(vals[13].chg_current_ua, 0, 65535 * 1000);
	zassert_between_inclusive(vals[14].chg_voltage_uv, 0, 65535 * 1000);
	/* Not testing props[15]. Battery status only carries status bits. */
	zassert_between_inclusive(vals[16].design_cap, 0, 65535);
	zassert_between_inclusive(vals[17].design_volt_mv, 0, 18000);
	zassert_between_inclusive(vals[18].sbs_at_rate, -32768, 32767);
	zassert_between_inclusive(vals[19].sbs_at_rate_time_to_full_mins, 0, 65535);
	zassert_between_inclusive(vals[20].sbs_at_rate_time_to_empty_mins, 0, 65535);
	/* Not testing props[21]. AtRateOK is a boolean. */
	zassert_between_inclusive(vals[22].sbs_remaining_capacity_alarm, 0, 65535);
	zassert_between_inclusive(vals[23].sbs_remaining_time_alarm_mins, 0, 65535);
	zassert_between_inclusive(vals[24].state_of_health, 0, 100);
#endif
}

/*
 * OperationStatus is requested through ManufacturerAccess rather than read directly, so this
 * works the same way on both parts and on a sealed pack.
 */
ZTEST_USER_F(bq41z50, test_get_charge_cutoff)
{
	union fuel_gauge_prop_val val;

	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CHARGE_CUTOFF, &val));
#if CONFIG_EMUL
	zassert_false(val.cutoff);
#endif
}

ZTEST_USER_F(bq41z50, test_get_operation_status)
{
	union fuel_gauge_prop_val val;

	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_FLAGS, &val));
#if CONFIG_EMUL
	zassert_equal(val.flags, EXP_OPERATION_STATUS);
#endif
}

/* Writes are restored so the test leaves a real pack's configuration untouched. */
ZTEST_USER_F(bq41z50, test_set_props__round_trip)
{
	union fuel_gauge_prop_val original;
	union fuel_gauge_prop_val probe;
	union fuel_gauge_prop_val readback;

	{
		zassert_ok(fuel_gauge_get_prop(fixture->dev,
					       FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM, &original));
		probe.sbs_remaining_capacity_alarm = 250;
		zassert_ok(fuel_gauge_set_prop(fixture->dev,
					       FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM, probe));
		zassert_ok(fuel_gauge_get_prop(fixture->dev,
					       FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM, &readback));
		zassert_equal(readback.sbs_remaining_capacity_alarm, 250);
		zassert_ok(fuel_gauge_set_prop(fixture->dev,
					       FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM, original));
	}
	{
		zassert_ok(fuel_gauge_get_prop(
			fixture->dev, FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS, &original));
		probe.sbs_remaining_time_alarm_mins = 15;
		zassert_ok(fuel_gauge_set_prop(fixture->dev,
					       FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS, probe));
		zassert_ok(fuel_gauge_get_prop(
			fixture->dev, FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS, &readback));
		zassert_equal(readback.sbs_remaining_time_alarm_mins, 15);
		zassert_ok(fuel_gauge_set_prop(fixture->dev,
					       FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS, original));
	}
	{
		/* AtRate is signed, so a negative probe covers the 16-bit sign conversion. */
		zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_SBS_ATRATE, &original));
		probe.sbs_at_rate = -500;
		zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_SBS_ATRATE, probe));
		zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_SBS_ATRATE, &readback));
		zassert_equal(readback.sbs_at_rate, -500);
		zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_SBS_ATRATE, original));
	}
	/*
	 * The other two writable properties are not round-tripped. ManufacturerAccess is a command
	 * register, so a write issues a subcommand and the following read returns that subcommand's
	 * result. BatteryMode only accepts writes to bits the pack is configured to expose, which
	 * varies between packs.
	 */
}

#if CONFIG_EMUL
/*
 * With CAPACITY_MODE set the gauge reports capacity in 10 mWh, which the uAh properties cannot
 * express, so the driver must refuse them rather than return a wrongly scaled value. Only the
 * bq41z50 opts into this behaviour, so the bq40z50 keeps reporting uAh unconditionally.
 */
ZTEST_USER_F(bq41z50, test_capacity_mode_switches_uah_support)
{
	union fuel_gauge_prop_val original;
	union fuel_gauge_prop_val mode;
	union fuel_gauge_prop_val val;

	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_SBS_MODE, &original));

	mode.sbs_mode = original.sbs_mode | BATTERY_MODE_CAPM;
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_SBS_MODE, mode));

	zassert_equal(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_REMAINING_CAPACITY_UAH, &val),
		      -ENOTSUP);
	zassert_equal(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH, &val),
		      -ENOTSUP);

	/* DesignCapacity is defined as mAh or 10 mWh, so it stays available. */
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_DESIGN_CAPACITY, &val));

	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_SBS_MODE, original));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_REMAINING_CAPACITY_UAH, &val));
	zassert_equal(val.remaining_capacity_uah, EXP_REMAINING_CAP_UAH);
}

/* Never run against real hardware: this powers the pack down. */
ZTEST_USER_F(bq41z50, test_battery_cutoff)
{
	zassert_ok(fuel_gauge_battery_cutoff(fixture->dev));
}
#endif /* CONFIG_EMUL */

ZTEST_SUITE(bq41z50, NULL, bq41z50_setup, NULL, NULL, NULL);
