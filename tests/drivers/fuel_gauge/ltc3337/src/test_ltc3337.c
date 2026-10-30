/*
 * Copyright (c) 2026 Federico Borgo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "emul_ltc3337.h"
#include "ltc3337.h"

#define LTC3337_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(adi_ltc3337)

BUILD_ASSERT(DT_PROP(LTC3337_NODE, ipeak_milliamp) == 100, "The tests assume IPEAK of 100 mA");

/* Charge of one counter LSB at 100 mA with the prescaler at 0 */
#define QLSB_UAH_X1000 14910000U
/* The die temperature code is 0.784 K wide, in dK */
#define TEMP_LSB_DK    8

struct ltc3337_fixture {
	const struct device *dev;
	const struct emul *emul;
};

static void *ltc3337_setup(void)
{
	static struct ltc3337_fixture fixture;

	fixture.dev = DEVICE_DT_GET(LTC3337_NODE);
	fixture.emul = EMUL_DT_GET(LTC3337_NODE);

	zassume_true(device_is_ready(fixture.dev), "Fuel gauge not ready");

	return &fixture;
}

ZTEST_F(ltc3337, test_init_state)
{
	union fuel_gauge_prop_val val;

	zassert_equal(ltc3337_emul_get_reg(fixture->emul, LTC3337_REG_A) & LTC3337_A_ALARM_MASK,
		      LTC3337_A_ALARM_MASK, "The alarm level should be at its maximum");

	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_STATUS, &val));
	/* IPK[2:0] = 111 reads back in bits 7:5 and nothing else is set */
	zassert_equal(val.fg_status, 0xE0U);

	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_VOLTAGE_UV, &val));
	zassert_equal(val.voltage_uv, 0);
}

ZTEST_F(ltc3337, test_voltage)
{
	union fuel_gauge_prop_val val;

	/* The unloaded BAT_IN voltage is in register E */
	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_E, 2458);
	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_D, 100);

	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_VOLTAGE_UV, &val));
	zassert_equal(val.voltage_uv, 2458 * 1465);

	/* The unused upper bits of the register must be ignored */
	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_E, 0xF000U | 2458U);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_VOLTAGE_UV, &val));
	zassert_equal(val.voltage_uv, 2458 * 1465);
}

ZTEST_F(ltc3337, test_temperature)
{
	union fuel_gauge_prop_val val;

	/* Code 0 is -41 degC */
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_TEMPERATURE_DK, &val));
	zassert_equal(val.temperature_dk, 2322);

	/* Code 0x55 is about 25 degC */
	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_C, (0x55U << 8) | 0xE0U);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_TEMPERATURE_DK, &val));
	zassert_equal(val.temperature_dk, 2988);

	/* Code 0xFF is 159 degC */
	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_C, (0xFFU << 8) | 0xE0U);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_TEMPERATURE_DK, &val));
	zassert_equal(val.temperature_dk, 4321);
}

ZTEST_F(ltc3337, test_charge)
{
	union fuel_gauge_prop_val val;

	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_B, 1000);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, &val));
	zassert_equal(val.consumed_capacity_uah, 1000U * (QLSB_UAH_X1000 / 1000U));

	/* Every prescaler step halves the charge of a count */
	val.cc_config = 2;
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, val));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, &val));
	zassert_equal(val.consumed_capacity_uah, 1000U * (QLSB_UAH_X1000 / 1000U) / 4U);

	/* The largest count does not overflow */
	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_B, UINT16_MAX);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, &val));
	zassert_equal(val.consumed_capacity_uah,
		      DIV_ROUND_CLOSEST(UINT16_MAX * (QLSB_UAH_X1000 / 1000U), 4U));
}

ZTEST_F(ltc3337, test_charge_rounding)
{
	union fuel_gauge_prop_val val = {.cc_config = 15};

	/* One count is 0.455 uAh at prescaler 15, the result is rounded to the nearest uAh */
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, val));

	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_B, 1);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, &val));
	zassert_equal(val.consumed_capacity_uah, 0U);

	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_B, 2);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, &val));
	zassert_equal(val.consumed_capacity_uah, 1U);

	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_B, 4);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, &val));
	zassert_equal(val.consumed_capacity_uah, 2U);
}

ZTEST_F(ltc3337, test_charge_preload)
{
	union fuel_gauge_prop_val val;

	/* 4096 counts, written in the upper byte of register B */
	val.consumed_capacity_uah = 4096U * (QLSB_UAH_X1000 / 1000U);
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, val));
	zassert_equal(ltc3337_emul_get_reg(fixture->emul, LTC3337_REG_B), 0x1000U);

	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, &val));
	zassert_equal(val.consumed_capacity_uah, 4096U * (QLSB_UAH_X1000 / 1000U));

	/* The lower byte of the counter is not writable, it is cut off */
	val.consumed_capacity_uah = 4100U * (QLSB_UAH_X1000 / 1000U);
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, val));
	zassert_equal(ltc3337_emul_get_reg(fixture->emul, LTC3337_REG_B), 0x1000U);

	/* More charge than the counter can hold */
	val.consumed_capacity_uah = UINT32_MAX;
	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, val),
		      -ERANGE);
}

ZTEST_F(ltc3337, test_cc_config)
{
	union fuel_gauge_prop_val val;
	const uint8_t config = LTC3337_CC_SHUTDOWN | LTC3337_CC_COUNTER_CHECK | 0x0AU;

	val.cc_config = config;
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, val));
	zassert_equal(ltc3337_emul_get_reg(fixture->emul, LTC3337_REG_A),
		      LTC3337_A_ALARM_MASK | config);

	val.cc_config = 0;
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, &val));
	zassert_equal(val.cc_config, config);

	/* Clear interrupt and ADC trigger are not part of the configuration */
	val.cc_config = LTC3337_A_CLEAR_INT;
	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, val), -EINVAL);
	val.cc_config = LTC3337_A_ADC_TRIGGER;
	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, val), -EINVAL);
}

ZTEST_F(ltc3337, test_temperature_alarms)
{
	union fuel_gauge_prop_val in;
	union fuel_gauge_prop_val out;

	in.high_temperature_alarm_dk = 3231; /* 50 degC */
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK, in));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK, &out));
	zassert_within(out.high_temperature_alarm_dk, in.high_temperature_alarm_dk, TEMP_LSB_DK);

	in.low_temperature_alarm_dk = 2631; /* -10 degC */
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_LOW_TEMPERATURE_ALARM_DK, in));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_LOW_TEMPERATURE_ALARM_DK, &out));
	zassert_within(out.low_temperature_alarm_dk, in.low_temperature_alarm_dk, TEMP_LSB_DK);

	/* The cold level is in the lower byte of register H and the hot level in the upper one */
	zassert_equal(ltc3337_emul_get_reg(fixture->emul, LTC3337_REG_H), (116U << 8) | 39U);

	/* Setting one level does not change the other */
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK, &out));
	zassert_within(out.high_temperature_alarm_dk, 3231, TEMP_LSB_DK);

	/* Colder than -41 degC and hotter than 159 degC cannot be represented */
	in.low_temperature_alarm_dk = 2000;
	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_LOW_TEMPERATURE_ALARM_DK, in),
		      -ERANGE);
	in.high_temperature_alarm_dk = 5000;
	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK, in),
		      -ERANGE);
}

ZTEST_F(ltc3337, test_status_acknowledges_alarms)
{
	union fuel_gauge_prop_val val;

	/* The alarm level was reached and the die is hot */
	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_C,
			     (0x55U << 8) | 0xE0U | LTC3337_C_ALARM_TRIP | LTC3337_C_HOT_ALARM);

	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_STATUS, &val));
	zassert_equal(val.fg_status, 0xE0U | LTC3337_C_ALARM_TRIP | LTC3337_C_HOT_ALARM);

	/* Reading the status released the interrupt */
	zassert_equal(ltc3337_emul_get_reg(fixture->emul, LTC3337_REG_C) & LTC3337_C_ALARMS, 0U);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_STATUS, &val));
	zassert_equal(val.fg_status, 0xE0U);

	/* The acknowledge did not touch the configuration */
	zassert_equal(ltc3337_emul_get_reg(fixture->emul, LTC3337_REG_A), LTC3337_A_POR);
}

ZTEST_F(ltc3337, test_unsupported_props)
{
	union fuel_gauge_prop_val val = {.avg_current_ua = 0};

	zassert_equal(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CURRENT_UA, &val), -ENOTSUP);
	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_VOLTAGE_UV, val), -ENOTSUP);
	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_STATUS, val), -ENOTSUP);

	/* The chip does not know the capacity of the battery, so it cannot tell what is left */
	zassert_equal(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_REMAINING_CAPACITY_UAH, &val),
		      -ENOTSUP);
	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_REMAINING_CAPACITY_UAH, val),
		      -ENOTSUP);
}

ZTEST_F(ltc3337, test_overflow_persists_until_counter_rewrite)
{
	union fuel_gauge_prop_val val;

	/* The counter wrapped, and the alarm level was reached on the way */
	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_C,
			     0xE0U | LTC3337_C_OVERFLOW | LTC3337_C_ALARM_TRIP);

	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_STATUS, &val));
	zassert_equal(val.fg_status, 0xE0U | LTC3337_C_OVERFLOW | LTC3337_C_ALARM_TRIP);

	/* Reading the status acknowledged the alarm trip, but not the overflow */
	zassert_equal(ltc3337_emul_get_reg(fixture->emul, LTC3337_REG_C) & LTC3337_C_ALARMS,
		      LTC3337_C_OVERFLOW);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_STATUS, &val));
	zassert_equal(val.fg_status, 0xE0U | LTC3337_C_OVERFLOW);

	/* Rewriting the counter recovers from the overflow */
	val.consumed_capacity_uah = 0;
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, val));
	zassert_equal(ltc3337_emul_get_reg(fixture->emul, LTC3337_REG_C) & LTC3337_C_ALARMS, 0U);
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_STATUS, &val));
	zassert_equal(val.fg_status, 0xE0U);
}

ZTEST_F(ltc3337, test_no_measurement_while_shut_down)
{
	union fuel_gauge_prop_val val = {.cc_config = LTC3337_CC_SHUTDOWN};

	ltc3337_emul_set_reg(fixture->emul, LTC3337_REG_E, 2458);

	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, val));

	/* The stored results are not updated in this mode, so they are not returned */
	zassert_equal(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_VOLTAGE_UV, &val), -ENODATA);
	zassert_equal(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_TEMPERATURE_DK, &val), -ENODATA);

	/* Everything that does not depend on a conversion keeps working */
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_STATUS, &val));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CONSUMED_CAPACITY_UAH, &val));

	val.cc_config = 0;
	zassert_ok(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_CC_CONFIG, val));
	zassert_ok(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_VOLTAGE_UV, &val));
	zassert_equal(val.voltage_uv, 2458 * 1465);
}

ZTEST_SUITE(ltc3337, NULL, ltc3337_setup, NULL, NULL, NULL);
