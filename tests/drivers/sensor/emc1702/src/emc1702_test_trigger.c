/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1702 simulator trigger test app
 *
 * The app simulates the chip and the I2C transactions in order to test
 * the triggers of the driver.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/emc1702.h>
#include <zephyr/ztest.h>

#include "emc1702.h"
#include "emul_emc1702.h"

#define EMC1702_NODE DT_NODELABEL(emc1702_simulate_test)

struct emc1702_trigger_fixture {
	const struct device *dev_basic;
	const struct emul *target_basic;
	const struct gpio_dt_spec alert_basic;
	uint8_t die_handler_calls;
	uint8_t amb_handler_calls;
	uint8_t curr_handler_calls;
	uint8_t volt_handler_calls;
	uint8_t fault_handler_calls;
};

static struct emc1702_trigger_fixture *current_fixture;

struct emc1702_trigger_cfg {
	struct gpio_dt_spec alerts;
};

static void *emc1702_trigger_setup(void)
{
	static struct emc1702_trigger_fixture fixture = {
		.dev_basic = DEVICE_DT_GET(EMC1702_NODE),
		.target_basic = EMUL_DT_GET(EMC1702_NODE),
		.alert_basic = GPIO_DT_SPEC_GET(EMC1702_NODE, alert_gpios),
	};

	zassert_not_null(fixture.dev_basic);
	zassert_not_null(fixture.target_basic);
	zassert_true(gpio_is_ready_dt(&fixture.alert_basic));

	return &fixture;
}

static void emc1702_trigger_before(void *f)
{
	struct emc1702_trigger_fixture *fixture = (struct emc1702_trigger_fixture *)f;

	current_fixture = fixture;
	fixture->die_handler_calls = 0;
	fixture->amb_handler_calls = 0;
	fixture->curr_handler_calls = 0;
	fixture->volt_handler_calls = 0;
	fixture->fault_handler_calls = 0;

	zassert_true(device_is_ready(fixture->dev_basic), "I2C device %s is not ready",
		     fixture->dev_basic->name);
}

ZTEST_SUITE(emc1702_trigger, NULL, emc1702_trigger_setup, emc1702_trigger_before, NULL, NULL);

static void on_alert_die(const struct device *dev, const struct sensor_trigger *trig)
{
	current_fixture->die_handler_calls++;
}

static void on_alert_amb(const struct device *dev, const struct sensor_trigger *trig)
{
	current_fixture->amb_handler_calls++;
}

static void on_alert_curr(const struct device *dev, const struct sensor_trigger *trig)
{
	current_fixture->curr_handler_calls++;
}

static void on_alert_volt(const struct device *dev, const struct sensor_trigger *trig)
{
	current_fixture->volt_handler_calls++;
}

static void on_alert_fault(const struct device *dev, const struct sensor_trigger *trig)
{
	current_fixture->fault_handler_calls++;
}
/**
 * @brief Test triggers
 */
ZTEST_F(emc1702_trigger, test_triggers)
{
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_HIGH_LIMIT_STATUS, 0xc3);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_LOW_LIMIT_STATUS, 0xc3);
	emc1702_emul_set_reg_8(fixture->target_basic, EMC1702_REG_EXT_DIODE_FAULT, 0x02);

	/* Register the alert handlers. */
	static const struct sensor_trigger trig_die = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_DIE_TEMP,
	};
	zassert_ok(sensor_trigger_set(fixture->dev_basic, &trig_die, on_alert_die));

	static const struct sensor_trigger trig_amb = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_AMBIENT_TEMP,
	};
	zassert_ok(sensor_trigger_set(fixture->dev_basic, &trig_amb, on_alert_amb));

	static const struct sensor_trigger trig_curr = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_CURRENT,
	};
	zassert_ok(sensor_trigger_set(fixture->dev_basic, &trig_curr, on_alert_curr));

	static const struct sensor_trigger trig_volt = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLTAGE,
	};
	zassert_ok(sensor_trigger_set(fixture->dev_basic, &trig_volt, on_alert_volt));

	static const struct sensor_trigger trig_fault = {
		.type = (enum sensor_trigger_type)SENSOR_TRIG_EMC1702_FAULT,
		.chan = (enum sensor_channel)SENSOR_CHAN_EMC1702_FAULT,
	};
	zassert_ok(sensor_trigger_set(fixture->dev_basic, &trig_fault, on_alert_fault));

	/* Assert the simulated pin. */
	zassert_ok(gpio_emul_input_set(fixture->alert_basic.port, fixture->alert_basic.pin, 0));
	k_sleep(K_MSEC(50));

	zassert_equal(current_fixture->die_handler_calls, 2, "Die Handler was called %d times",
		      current_fixture->die_handler_calls);
	zassert_equal(current_fixture->amb_handler_calls, 2, "Amb Handler was called %d times",
		      current_fixture->amb_handler_calls);
	zassert_equal(current_fixture->curr_handler_calls, 2, "Curr Handler was called %d times",
		      current_fixture->curr_handler_calls);
	zassert_equal(current_fixture->volt_handler_calls, 2, "Volt Handler was called %d times",
		      current_fixture->volt_handler_calls);
	zassert_equal(current_fixture->fault_handler_calls, 1, "Fault Handler was called %d times",
		      current_fixture->fault_handler_calls);
}
