/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1702 simulator test app
 *
 * The app simulates the chip and the I2C transactions in order to test
 * the capability of the driver to translate raw register values
 * into their physical equivalent.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "emc1702.h"
#include "emul_emc1702.h"

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
	zexpect_within(20000000, internal, 10000000, "Got %.6f C", internal);
	zexpect_within(40000000, external, 10000000, "Got %.6f C", external);

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
	zexpect_within(-60000000, internal, 10000000, "Got %.6f C", internal);
	zexpect_within(-130000000, external, 10000000, "Got %.6f C", external);

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
	zexpect_within(-125000, internal, 5000, "Got %.6f C", internal);
	zexpect_within(125000, external, 500, "Got %.6f C", external);
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
	 * 0470h = 0.0346 A
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SENSE_VOLTAGE_HIGH, 0x04);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SENSE_VOLTAGE_LOW, 0x70);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_CURRENT, &current_val));
	current = sensor_value_to_micro(&current_val);
	zexpect_within(33000, current, 5000, "Got %.6f A", current);

	/*
	 * Test negative values
	 *
	 * 9a20h = -0.7958 A
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SENSE_VOLTAGE_HIGH, 0x9a);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SENSE_VOLTAGE_LOW, 0x20);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_CURRENT, &current_val));
	current = sensor_value_to_micro(&current_val);
	zexpect_within(-800000, current, 100000, "Got %.6f A", current);
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
	 * 2280h = 3.232 V
	 */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SOURCE_VOLTAGE_HIGH, 0x22);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SOURCE_VOLTAGE_LOW, 0x80);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_VOLTAGE, &voltage_val));
	voltage = sensor_value_to_micro(&voltage_val);
	zexpect_within(3000000, voltage, 300000, "Got %.6f A", voltage);

	/* 48a0h = 6.805 V */
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SOURCE_VOLTAGE_HIGH, 0x48);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_SOURCE_VOLTAGE_LOW, 0xa0);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_VOLTAGE, &voltage_val));
	voltage = sensor_value_to_micro(&voltage_val);
	zexpect_within(6000000, voltage, 1000000, "Got %.6f A", voltage);
}
