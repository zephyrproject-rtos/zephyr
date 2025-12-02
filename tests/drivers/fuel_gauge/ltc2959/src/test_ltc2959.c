/*
 * Copyright (c) 2025 Nathan Winslow <natelostintimeandspace@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#define LTC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(adi_ltc2959)
BUILD_ASSERT(DT_NODE_EXISTS(LTC_NODE), "No adi,ltc2959 node in DT for tests");

#define RSENSE_MOHMS DT_PROP(LTC_NODE, rsense_milliohms)

/* Integer LSB sizes (keep tests stable) */
#define CURRENT_LSB_UA (97500000ULL / ((uint64_t)RSENSE_MOHMS * 32768ULL))
#define VOLTAGE_MAX_UV (UINT16_MAX * 955U) /* ~955 = 62.6V full scale / 65536 */

struct ltc2959_fixture {
	const struct device *dev;
	const struct fuel_gauge_driver_api *api;
};

static void *ltc2959_setup(void)
{
	static ZTEST_DMEM struct ltc2959_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(adi_ltc2959);
	k_object_access_all_grant(fixture.dev);

	zassume_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

LOG_MODULE_REGISTER(test_ltc2959, LOG_LEVEL_INF);

ZTEST_F(ltc2959, test_get_props__returns_ok)
{
	fuel_gauge_prop_t props[] = {
		FUEL_GAUGE_STATUS,
		FUEL_GAUGE_VOLTAGE,
		FUEL_GAUGE_CURRENT,
		FUEL_GAUGE_TEMPERATURE,
	};

	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];
	int ret = fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props));

#if CONFIG_EMUL
	zassert_equal(vals[0].fg_status, 0x01);
	zassert_equal(vals[1].voltage, 0x00);
	zassert_equal(vals[2].current, 0x00);
	zassert_equal(vals[3].temperature, 0x00);
#else
	zassert_between_inclusive(vals[0].fg_status, 0, 0xFF);
	zassert_between_inclusive(vals[1].voltage, 0, VOLTAGE_MAX_UV);
#endif
	zassert_equal(ret, 0, "Getting bad property has a good status.");
}

ZTEST_F(ltc2959, test_set_get_single_prop)
{
	int ret;
	union fuel_gauge_prop_val in = {.low_voltage_alarm = 1200000}; /* 1.2V */

	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_LOW_VOLTAGE_ALARM, in);
	zassert_equal(ret, 0, "set low voltage threshold failed");

	union fuel_gauge_prop_val out;

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_LOW_VOLTAGE_ALARM, &out);
	zassert_equal(ret, 0, "get low voltage threshold failed");

	/* Allow for register quantization: one LSB ≈ 1.91 mV */
	const int32_t lsb_uv = 62600000 / 32768; /* integer ≈ 1910 */
	int32_t diff = (int32_t)out.low_voltage_alarm - (int32_t)in.low_voltage_alarm;

	zassert_true(diff <= lsb_uv && diff >= -lsb_uv,
		     "Set/get mismatch: in=%d, out=%d, diff=%d > LSB=%d", (int)in.low_voltage_alarm,
		     (int)out.low_voltage_alarm, (int)(diff), (int)lsb_uv);

	LOG_INF("in=%d, out=%d, diff=%d > LSB=%d", (int)in.low_voltage_alarm,
		(int)out.low_voltage_alarm, (int)(diff), (int)lsb_uv);
}

ZTEST_F(ltc2959, test_current_threshold_roundtrip)
{
	int ret;
	union fuel_gauge_prop_val in, out;
	int32_t tol = CURRENT_LSB_UA ? (int32_t)CURRENT_LSB_UA : 100;

	in.high_current_alarm = 123456; /* µA */
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_HIGH_CURRENT_ALARM, in);
	zassert_equal(ret, 0, "set current high threshold failed (%d)", ret);

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_HIGH_CURRENT_ALARM, &out);
	zassert_equal(ret, 0, "get current high threshold failed (%d)", ret);

	int32_t diff = out.high_current_alarm - in.high_current_alarm;

	if (diff < 0) {
		diff = -diff;
	}

	zassert_true(diff <= tol, "current high threshold mismatch: in=%d out=%d diff=%d tol=%d",
		     (int)in.high_current_alarm, (int)out.high_current_alarm, (int)diff, (int)tol);

	in.low_current_alarm = -78901; /* µA */
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_LOW_CURRENT_ALARM, in);
	zassert_equal(ret, 0, "set current low threshold failed (%d)", ret);

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_LOW_CURRENT_ALARM, &out);
	zassert_equal(ret, 0, "get current low threshold failed (%d)", ret);

	diff = out.low_current_alarm - in.low_current_alarm;

	if (diff < 0) {
		diff = -diff;
	}

	zassert_true(diff <= tol, "current low threshold mismatch: in=%d out=%d diff=%d tol=%d",
		     (int)in.low_current_alarm, (int)out.low_current_alarm, (int)diff, (int)tol);
}

ZTEST_F(ltc2959, test_temperature_threshold_roundtrip)
{
	int ret;
	union fuel_gauge_prop_val in;
	union fuel_gauge_prop_val out;

	in.low_temperature_alarm = 3000;
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_LOW_TEMPERATURE_ALARM, in);
	zassert_equal(ret, 0, "set temp low threshold failed (%d)", ret);

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_LOW_TEMPERATURE_ALARM, &out);
	zassert_equal(ret, 0, "get temp low threshold failed (%d)", ret);
	int32_t diff = (int32_t)out.low_temperature_alarm - (int32_t)in.low_temperature_alarm;

	if (diff < 0) {
		diff = -diff;
	}

	zassert_true(diff <= 1, "temp low threshold mismatch: in=%u out=%u diff=%d",
		     in.low_temperature_alarm, out.low_temperature_alarm, (int)diff);

	in.high_temperature_alarm = 3500;
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_HIGH_TEMPERATURE_ALARM, in);
	zassert_equal(ret, 0, "set temp high threshold failed (%d)", ret);

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_HIGH_TEMPERATURE_ALARM, &out);
	zassert_equal(ret, 0, "get temp high threshold failed (%d)", ret);
	diff = (int32_t)out.high_temperature_alarm - (int32_t)in.high_temperature_alarm;

	if (diff < 0) {
		diff = -diff;
	}

	zassert_true(diff <= 1, "temp high threshold mismatch: in=%u out=%u diff=%d",
		     in.high_temperature_alarm, out.high_temperature_alarm, (int)diff);
}

ZTEST_F(ltc2959, test_adc_mode_roundtrip)
{
	int ret;
	union fuel_gauge_prop_val in, out;

	in.adc_mode = 0xC0 | 0x10; /* CONT_VIT + GPIO BIPOLAR */
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_ADC_MODE, in);
	zassert_equal(ret, 0, "set ADC_MODE failed (%d)", ret);

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_ADC_MODE, &out);
	zassert_equal(ret, 0, "get ADC_MODE failed (%d)", ret);
	zassert_equal(out.adc_mode, in.adc_mode, "ADC_MODE mismatch (got 0x%02x)", out.adc_mode);
}

ZTEST_F(ltc2959, test_remaining_capacity_roundtrip)
{
	int ret;
	union fuel_gauge_prop_val in, out;

	in.remaining_capacity = 1234567; /* µAh */
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_REMAINING_CAPACITY, in);
	zassert_equal(ret, 0, "set ACR failed (%d)", ret);

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_REMAINING_CAPACITY, &out);
	zassert_equal(ret, 0, "get ACR failed (%d)", ret);

	int32_t diff = (int32_t)out.remaining_capacity - (int32_t)in.remaining_capacity;

	if (diff < 0) {
		diff = -diff;
	}

	zassert_true(diff <= 1, "ACR mismatch: in=%d out=%d diff=%d tol=1",
		     (int)in.remaining_capacity, (int)out.remaining_capacity, (int)diff);
}

ZTEST_F(ltc2959, test_remaining_capacity_reserved_guard)
{
	int ret;
	union fuel_gauge_prop_val in, out;

	/* 0xFFFFFFFF counts ≈ 2,289,000,000 µAh (533 nAh/LSB) */
	in.remaining_capacity = 2289000000U;
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_REMAINING_CAPACITY, in);
	zassert_equal(ret, 0, "set ACR near fullscale failed (%d)", ret);

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_REMAINING_CAPACITY, &out);
	zassert_equal(ret, 0, "get ACR near fullscale failed (%d)", ret);

	/* We expect the driver to write 0xFFFFFFFE instead, so out <= in and close */
	zassert_true(out.remaining_capacity <= in.remaining_capacity,
		     "ACR guard failed: got larger than requested");
	int32_t diff = (int32_t)in.remaining_capacity - (int32_t)out.remaining_capacity;

	if (diff < 0) {
		diff = -diff;
	}

	zassert_true(diff <= 1, "ACR guard too lossy: in=%d out=%d |diff|=%d",
		     (int)in.remaining_capacity, (int)out.remaining_capacity, (int)diff);
}

ZTEST_F(ltc2959, test_cc_config_sanitized)
{
	int ret;
	union fuel_gauge_prop_val in, out;

	in.cc_config = 0xFF; /* try to set everything */
	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, in);
	zassert_equal(ret, 0, "set cc_config failed (%d)", ret);

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, &out);
	zassert_equal(ret, 0, "get cc_config failed (%d)", ret);

	/* Expect bits 7,6,3 kept; bit 4 forced; others cleared => 0xD8 */
	zassert_equal(out.cc_config, 0xD8, "cc_config not sanitized (got 0x%02X)", out.cc_config);
}

ZTEST_USER_F(ltc2959, test_get_some_props_failed__returns_bad_status)
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

ZTEST_F(ltc2959, test_set_some_props_failed__returns_err)
{
	fuel_gauge_prop_t prop_types[] = {
		/* First invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Second invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Valid property */
		FUEL_GAUGE_LOW_VOLTAGE_ALARM,
	};

	union fuel_gauge_prop_val props[] = {
		/* First invalid property */
		{0},
		/* Second invalid property */
		{0},
		/* Valid property */
		{.voltage = 0},
	};

	int ret = fuel_gauge_set_props(fixture->dev, prop_types, props, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP);
}

ZTEST_SUITE(ltc2959, NULL, ltc2959_setup, NULL, NULL, NULL);
