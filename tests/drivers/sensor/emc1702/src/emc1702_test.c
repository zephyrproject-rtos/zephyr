/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1702 simulator test app
 *
 * The app simulates the chip and the I2C transactions in order to test
 * the capabilities of the driver.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/emc1702.h>
#include <zephyr/ztest.h>

#include "emc1702.h"
#include "emul_emc1702.h"

#define UPPER_LIMIT_AMB 30.5
#define LOWER_LIMIT_AMB -5.625
#define CRIT_LIMIT_AMB  40
#define UPPER_LIMIT_DIE 26
#define LOWER_LIMIT_DIE -13
#define CRIT_LIMIT_DIE  35
#define TEMP_HYSTERESIS 3

#define UPPER_LIMIT_CURR 0.5
#define LOWER_LIMIT_CURR -0.5
#define CRIT_LIMIT_CURR  0.75
#define CURR_HYSTERESIS  0.025

#define UPPER_LIMIT_VOLT 10
#define LOWER_LIMIT_VOLT 1
#define CRIT_LIMIT_VOLT  20
#define VOLT_HYSTERESIS  2

struct emc1702_fixture {
	const struct device *dev_basic;
	const struct emul *target_basic;
};

static void *emc1702_setup(void)
{
	static struct emc1702_fixture fixture = {
		.dev_basic = DEVICE_DT_GET(DT_NODELABEL(emc1702_simulate_test)),
		.target_basic = EMUL_DT_GET(DT_NODELABEL(emc1702_simulate_test)),
	};

	zassert_not_null(fixture.dev_basic);
	zassert_not_null(fixture.target_basic);

	return &fixture;
}

static void emc1702_before(void *f)
{
	struct emc1702_fixture *fixture = (struct emc1702_fixture *)f;

	zassert_true(device_is_ready(fixture->dev_basic), "I2C device %s is not ready",
		     fixture->dev_basic->name);
}

ZTEST_SUITE(emc1702, NULL, emc1702_setup, emc1702_before, NULL, NULL);

/**
 * @brief Test temperature reading
 */
ZTEST_F(emc1702, test_temp_read)
{
	struct sensor_value internal_val, external_val;
	int64_t internal, external;

	/*
	 * Set internal and external diode raw temp
	 *
	 * Datasheet Table 5.3 explains the temperature data format
	 *
	 * 1820h = 24.125°C
	 * 30e0h = 48.875°C
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_INTERNAL_DIODE_HIGH, 0x18);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_INTERNAL_DIODE_LOW, 0x20);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_EXTERNAL_DIODE_HIGH, 0x30);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_EXTERNAL_DIODE_LOW, 0xe0);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP, &internal_val));
	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_AMBIENT_TEMP, &external_val));
	internal = sensor_value_to_micro(&internal_val);
	external = sensor_value_to_micro(&external_val);
	zassert_equal(internal, 24125000, "Got %lld µºC", internal);
	zassert_equal(external, 48875000, "Got %lld µºC", external);

	/*
	 * Test negative values
	 *
	 * c100h = -63°C
	 * 85a0h = -122.375°C
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_INTERNAL_DIODE_HIGH, 0xc1);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_INTERNAL_DIODE_LOW, 0x00);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_EXTERNAL_DIODE_HIGH, 0x85);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_EXTERNAL_DIODE_LOW, 0xa0);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP, &internal_val));
	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_AMBIENT_TEMP, &external_val));
	internal = sensor_value_to_micro(&internal_val);
	external = sensor_value_to_micro(&external_val);
	zassert_equal(internal, -63000000, "Got %lld µºC", internal);
	zassert_equal(external, -122375000, "Got %lld µºC", external);

	/*
	 * ffe0h = -0.125°C
	 * 0020h = 0.125°C
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_INTERNAL_DIODE_HIGH, 0xff);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_INTERNAL_DIODE_LOW, 0xe0);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_EXTERNAL_DIODE_HIGH, 0x00);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_EXTERNAL_DIODE_LOW, 0x20);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP, &internal_val));
	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_AMBIENT_TEMP, &external_val));
	internal = sensor_value_to_micro(&internal_val);
	external = sensor_value_to_micro(&external_val);
	zassert_equal(internal, -125000, "Got %lld µºC", internal);
	zassert_equal(external, 125000, "Got %lld µºC", external);
}

/**
 * @brief Test current reading
 */
ZTEST_F(emc1702, test_current_read)
{
	struct sensor_value current_val;
	int64_t current;

	/*
	 * Set raw current
	 *
	 * Datasheet Table 5.37 explains the current data format
	 *
	 * 0470h = 0.034667 A
	 * This transformation is valid only for sense resistor 10 mOhm
	 * and current sense range 10 mV.
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SENSE_VOLTAGE_HIGH, 0x04);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SENSE_VOLTAGE_LOW, 0x70);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_CURRENT, &current_val));
	current = sensor_value_to_micro(&current_val);
	zassert_equal(current, 34667, "Got %lld µA", current);

	/*
	 * Test negative values
	 *
	 * 9a20h = -0.795898 A
	 * This transformation is valid only for sense resistor 10 mOhm
	 * and current sense range 10 mV.
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SENSE_VOLTAGE_HIGH, 0x9a);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SENSE_VOLTAGE_LOW, 0x20);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_CURRENT, &current_val));
	current = sensor_value_to_micro(&current_val);
	zassert_equal(current, -795898, "Got %lld µA", current);
}

/**
 * @brief Test voltage reading
 */
ZTEST_F(emc1702, test_voltage_read)
{
	struct sensor_value voltage_val;
	int64_t voltage;

	/*
	 * Set raw voltage
	 *
	 * 2280h = 3.232798 V
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SOURCE_VOLTAGE_HIGH, 0x22);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SOURCE_VOLTAGE_LOW, 0x80);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_VOLTAGE, &voltage_val));
	voltage = sensor_value_to_micro(&voltage_val);
	zassert_equal(voltage, 3232798, "Got %lld µV", voltage);

	/* 48a0h = 6.805274 V */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SOURCE_VOLTAGE_HIGH, 0x48);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SOURCE_VOLTAGE_LOW, 0xa0);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_VOLTAGE, &voltage_val));
	voltage = sensor_value_to_micro(&voltage_val);
	zassert_equal(voltage, 6805274, "Got %lld µV", voltage);
}

/**
 * @brief Test power ratio reading
 */
ZTEST_F(emc1702, test_power_read)
{
	struct sensor_value power_val;
	int64_t power;

	/*
	 * Set raw power
	 *
	 * 131h = 0.111641 W
	 * This transformation is valid only for sense resistor 10 mOhm
	 * and current sense range 10 mV.
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_POWER_RATIO_HIGH, 0x01);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_POWER_RATIO_LOW, 0x31);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_POWER, &power_val));
	power = sensor_value_to_micro(&power_val);

	zassert_equal(power, 111641, "Got %lld µW", power);

	/*
	 * Set raw power
	 *
	 * 2000h = 2.998583 W
	 * This transformation is valid only for sense resistor 10 mOhm
	 * and current sense range 10mV.
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_POWER_RATIO_HIGH, 0x20);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_POWER_RATIO_LOW, 0x00);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_POWER, &power_val));
	power = sensor_value_to_micro(&power_val);

	zassert_equal(power, 2998583, "Got %lld µW", power);
}

/**
 * @brief Test fault reading
 */
ZTEST_F(emc1702, test_fault_read)
{
	struct sensor_value fault_val;

	/* Test without fault. */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_EXT_DIODE_FAULT, 0x00);
	zassert_ok(sensor_sample_fetch(fixture->dev_basic));
	zassert_ok(sensor_channel_get(fixture->dev_basic,
				      (enum sensor_channel)SENSOR_CHAN_EMC1702_FAULT, &fault_val));
	zassert_equal(fault_val.val1, 0, "Got %d", fault_val.val1);

	/* Test with fault. */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_EXT_DIODE_FAULT, 0x02);
	zassert_ok(sensor_sample_fetch(fixture->dev_basic));
	zassert_ok(sensor_channel_get(fixture->dev_basic,
				      (enum sensor_channel)SENSOR_CHAN_EMC1702_FAULT, &fault_val));
	zassert_equal(fault_val.val1, 2, "Got %d", fault_val.val1);
}

/**
 * @brief Test ambient temperature attributes
 */
ZTEST_F(emc1702, test_ambient_temperature_attr)
{
	struct sensor_value upper, high_lim, lower, low_lim, critical, crit_lim;

	zassert_ok(sensor_value_from_double(&upper, UPPER_LIMIT_AMB));
	zassert_ok(sensor_value_from_double(&lower, LOWER_LIMIT_AMB));
	zassert_ok(sensor_value_from_double(&critical, CRIT_LIMIT_AMB));

	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_AMBIENT_TEMP,
				   SENSOR_ATTR_UPPER_THRESH, &upper));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_AMBIENT_TEMP,
				   SENSOR_ATTR_LOWER_THRESH, &lower));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_AMBIENT_TEMP,
				   (enum sensor_attribute)SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
				   &critical));

	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_AMBIENT_TEMP,
				   SENSOR_ATTR_UPPER_THRESH, &high_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_AMBIENT_TEMP,
				   SENSOR_ATTR_LOWER_THRESH, &low_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_AMBIENT_TEMP,
				   (enum sensor_attribute)SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
				   &crit_lim));

	zassert_equal(sensor_value_to_micro(&upper), sensor_value_to_micro(&high_lim),
		      "Got %lld µºC", sensor_value_to_micro(&high_lim));
	zassert_equal(sensor_value_to_micro(&lower), sensor_value_to_micro(&low_lim),
		      "Got %lld µºC", sensor_value_to_micro(&low_lim));
	zassert_equal(sensor_value_to_micro(&critical), sensor_value_to_micro(&crit_lim),
		      "Got %lld µºC", sensor_value_to_micro(&crit_lim));
}

/**
 * @brief Test die temperature attributes
 */
ZTEST_F(emc1702, test_die_temperature_attr)
{
	struct sensor_value upper, high_lim, lower, low_lim, critical, crit_lim;

	zassert_ok(sensor_value_from_double(&upper, UPPER_LIMIT_DIE));
	zassert_ok(sensor_value_from_double(&lower, LOWER_LIMIT_DIE));
	zassert_ok(sensor_value_from_double(&critical, CRIT_LIMIT_DIE));

	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP,
				   SENSOR_ATTR_UPPER_THRESH, &upper));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP,
				   SENSOR_ATTR_LOWER_THRESH, &lower));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP,
				   (enum sensor_attribute)SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
				   &critical));

	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP,
				   SENSOR_ATTR_UPPER_THRESH, &high_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP,
				   SENSOR_ATTR_LOWER_THRESH, &low_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP,
				   (enum sensor_attribute)SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
				   &crit_lim));

	zassert_equal(sensor_value_to_micro(&upper), sensor_value_to_micro(&high_lim),
		      "Got %lld µºC", sensor_value_to_micro(&high_lim));
	zassert_equal(sensor_value_to_micro(&lower), sensor_value_to_micro(&low_lim),
		      "Got %lld µºC", sensor_value_to_micro(&low_lim));
	zassert_equal(sensor_value_to_micro(&critical), sensor_value_to_micro(&crit_lim),
		      "Got %lld µºC", sensor_value_to_micro(&crit_lim));
}

/**
 * Test temperature hysteresis
 */
ZTEST_F(emc1702, test_hyst_temp)
{
	struct sensor_value hyst, hyst_read;

	zassert_ok(sensor_value_from_double(&hyst, TEMP_HYSTERESIS));

	zassert_ok(sensor_attr_set(fixture->dev_basic,
				   (enum sensor_channel)SENSOR_CHAN_EMC1702_TEMP_HYSTERESIS,
				   SENSOR_ATTR_HYSTERESIS, &hyst));

	zassert_ok(sensor_attr_get(fixture->dev_basic,
				   (enum sensor_channel)SENSOR_CHAN_EMC1702_TEMP_HYSTERESIS,
				   SENSOR_ATTR_HYSTERESIS, &hyst_read));

	zassert_equal(hyst.val1, hyst_read.val1, "Got hysteresis = %d ºC", hyst_read.val1);
}

/**
 * @brief Test current attributes
 */
ZTEST_F(emc1702, test_current_attr)
{
	struct sensor_value upper, high_lim, lower, low_lim, critical, crit_lim, hyst, hyst_read;

	zassert_ok(sensor_value_from_double(&upper, UPPER_LIMIT_CURR));
	zassert_ok(sensor_value_from_double(&lower, LOWER_LIMIT_CURR));
	zassert_ok(sensor_value_from_double(&critical, CRIT_LIMIT_CURR));
	zassert_ok(sensor_value_from_double(&hyst, CURR_HYSTERESIS));

	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_CURRENT,
				   SENSOR_ATTR_UPPER_THRESH, &upper));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_CURRENT,
				   SENSOR_ATTR_LOWER_THRESH, &lower));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_CURRENT,
				   (enum sensor_attribute)SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
				   &critical));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_CURRENT, SENSOR_ATTR_HYSTERESIS,
				   &hyst));

	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_CURRENT,
				   SENSOR_ATTR_UPPER_THRESH, &high_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_CURRENT,
				   SENSOR_ATTR_LOWER_THRESH, &low_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_CURRENT,
				   (enum sensor_attribute)SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
				   &crit_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_CURRENT, SENSOR_ATTR_HYSTERESIS,
				   &hyst_read));

	zassert_within(sensor_value_to_micro(&upper), sensor_value_to_micro(&high_lim), 5000,
		       "Got %lld µA", sensor_value_to_micro(&high_lim));
	zassert_within(sensor_value_to_micro(&lower), sensor_value_to_micro(&low_lim), 5000,
		       "Got %lld µA", sensor_value_to_micro(&low_lim));
	zassert_within(sensor_value_to_micro(&critical), sensor_value_to_micro(&crit_lim), 5000,
		       "Got %lld µA", sensor_value_to_micro(&crit_lim));
	zassert_within(sensor_value_to_micro(&hyst), sensor_value_to_micro(&hyst_read), 5000,
		       "Got hysteresis = %lld µA", sensor_value_to_micro(&hyst_read));
}

/**
 * @brief Test voltage attributes
 */
ZTEST_F(emc1702, test_voltage_attr)
{
	struct sensor_value upper, high_lim, lower, low_lim, critical, crit_lim, hyst, hyst_read;

	zassert_ok(sensor_value_from_double(&upper, UPPER_LIMIT_VOLT));
	zassert_ok(sensor_value_from_double(&lower, LOWER_LIMIT_VOLT));
	zassert_ok(sensor_value_from_double(&critical, CRIT_LIMIT_VOLT));
	zassert_ok(sensor_value_from_double(&hyst, VOLT_HYSTERESIS));

	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_VOLTAGE,
				   SENSOR_ATTR_UPPER_THRESH, &upper));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_VOLTAGE,
				   SENSOR_ATTR_LOWER_THRESH, &lower));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_VOLTAGE,
				   (enum sensor_attribute)SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
				   &critical));
	zassert_ok(sensor_attr_set(fixture->dev_basic, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_HYSTERESIS,
				   &hyst));

	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_VOLTAGE,
				   SENSOR_ATTR_UPPER_THRESH, &high_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_VOLTAGE,
				   SENSOR_ATTR_LOWER_THRESH, &low_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_VOLTAGE,
				   (enum sensor_attribute)SENSOR_ATTR_EMC1702_CRITICAL_LIMIT,
				   &crit_lim));
	zassert_ok(sensor_attr_get(fixture->dev_basic, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_HYSTERESIS,
				   &hyst_read));

	zassert_within(sensor_value_to_micro(&upper), sensor_value_to_micro(&high_lim), 100000,
		       "Got %lld µV", sensor_value_to_micro(&high_lim));
	zassert_within(sensor_value_to_micro(&lower), sensor_value_to_micro(&low_lim), 100000,
		       "Got %lld µV", sensor_value_to_micro(&low_lim));
	zassert_within(sensor_value_to_micro(&critical), sensor_value_to_micro(&crit_lim), 100000,
		       "Got %lld µV", sensor_value_to_micro(&crit_lim));
	zassert_within(sensor_value_to_micro(&hyst), sensor_value_to_micro(&hyst_read), 100000,
		       "Got hysteresis = %lld µV", sensor_value_to_micro(&hyst_read));
}

/**
 * @brief Test limit status reading
 */
ZTEST_F(emc1702, test_limit_status_read)
{
	struct sensor_value high_status, low_status, crit_status;
	int rc;

	/*
	 * Set values in status limit registers.
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_HIGH_LIMIT_STATUS, 0xc3);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_LOW_LIMIT_STATUS, 0x81);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_CRIT_LIMIT_STATUS, 0x03);

	if (IS_ENABLED(CONFIG_EMC1702_TRIGGER)) {
		rc = sensor_attr_get(
			fixture->dev_basic, (enum sensor_channel)SENSOR_CHAN_EMC1702_LIMIT_STATUS,
			(enum sensor_attribute)SENSOR_ATTR_EMC1702_HIGH_LIMIT_STATUS, &high_status);
		zassert_equal(rc, -ENOTSUP, "Got error %d", rc);
		rc = sensor_attr_get(
			fixture->dev_basic, (enum sensor_channel)SENSOR_CHAN_EMC1702_LIMIT_STATUS,
			(enum sensor_attribute)SENSOR_ATTR_EMC1702_LOW_LIMIT_STATUS, &low_status);
		zassert_equal(rc, -ENOTSUP, "Got error %d", rc);
	} else {
		zassert_ok(sensor_attr_get(
			fixture->dev_basic, (enum sensor_channel)SENSOR_CHAN_EMC1702_LIMIT_STATUS,
			(enum sensor_attribute)SENSOR_ATTR_EMC1702_HIGH_LIMIT_STATUS,
			&high_status));
		zassert_ok(sensor_attr_get(
			fixture->dev_basic, (enum sensor_channel)SENSOR_CHAN_EMC1702_LIMIT_STATUS,
			(enum sensor_attribute)SENSOR_ATTR_EMC1702_LOW_LIMIT_STATUS, &low_status));

		zassert_equal(high_status.val1, 195, "Got status = %d", high_status);
		zassert_equal(low_status.val1, 129, "Got status = %d", low_status);
	}

	zassert_ok(sensor_attr_get(
		fixture->dev_basic, (enum sensor_channel)SENSOR_CHAN_EMC1702_LIMIT_STATUS,
		(enum sensor_attribute)SENSOR_ATTR_EMC1702_CRIT_LIMIT_STATUS, &crit_status));
	zassert_equal(crit_status.val1, 3, "Got status = %d", crit_status);
}
