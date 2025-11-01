/*
 * Copyright (c) 2023 North River Systems Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/dt-bindings/sensor/ina230.h>
#include <zephyr/drivers/sensor/ina230.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ina230_emul.h>
#include <ina230.h>

DEFINE_FFF_GLOBALS;

enum ina23x_ids {
	INA230,
	INA236
};

struct ina230_fixture {
	const struct device *dev;
	const struct emul *mock;
	const uint16_t current_lsb_uA;
	const uint16_t rshunt_uOhms;
	const uint16_t config;
	const enum ina23x_ids dev_type;
	const struct gpio_dt_spec alert_gpios;
};

/**
 * @brief Verify devicetree default configuration is correct.
 */
ZTEST(ina230_0, test_default_config)
{
	const struct ina2xx_config *config;
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ina230_default_test));

	zassert_not_null(dev);
	config = dev->config;
	zassert_not_null(config);

	/* confirm default DT configuration */
	uint16_t expected = 0x0127;

	zexpect_equal(expected, config->config, "0x%x != config (0x%x)",
		expected, config->config);
}

static void test_datasheet_example(struct ina230_fixture *fixture)
{
	struct sensor_value sensor_val;
	uint16_t raw_voltage, raw_current, raw_power;
	double actual;

	/* only run test for datasheet example of 1mA current LSB and 2 mOhm shunt */
	if (fixture->current_lsb_uA != 1000 || fixture->rshunt_uOhms != 2000) {
		ztest_test_skip();
	}

	if (fixture->dev_type == INA230) {
		raw_voltage = 9584;
		raw_current = 10000;
		raw_power = 4792;
	} else {
		raw_voltage = 7487;
		raw_current = 10000;
		raw_power = 3744;
	}

	ina230_mock_set_register(fixture->mock->data, INA230_REG_BUS_VOLT, raw_voltage);
	ina230_mock_set_register(fixture->mock->data, INA230_REG_CURRENT, raw_current);
	ina230_mock_set_register(fixture->mock->data, INA230_REG_POWER, raw_power);
	zassert_ok(sensor_sample_fetch(fixture->dev));

	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_VOLTAGE, &sensor_val));
	actual = sensor_value_to_double(&sensor_val);
	zexpect_within(11.98, actual, 1.25e-3, "Expected 11.98 V, got %.6f V", actual);

	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_CURRENT, &sensor_val));
	actual = sensor_value_to_double(&sensor_val);
	zexpect_within(10.0, actual, 1e-3, "Expected 10 A, got %.6f V", actual);

	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_POWER, &sensor_val));
	actual = sensor_value_to_double(&sensor_val);
	zexpect_within(119.82, actual, 25e-3, "Expected 119.82 W, got %.6f W", actual);
}

/**
 * @brief Verify bus voltages for all ina230 nodes in DT
 *
 * @param fixture
 */
static void test_shunt_cal(struct ina230_fixture *fixture)
{
	/* Confirm SHUNT_CAL register which is 5120e-6 / Current_LSB * Rshunt */
	double shunt_cal = 5120e-6 / (fixture->current_lsb_uA * 1e-6 *
		fixture->rshunt_uOhms * 1e-6);

	uint32_t shunt_register_actual;
	uint16_t shunt_register_expected = (uint16_t)shunt_cal;

	zassert_ok(ina230_mock_get_register(fixture->mock->data, INA230_REG_CALIB,
		&shunt_register_actual));
	zexpect_within(shunt_register_expected, shunt_register_actual, 1,
		"Expected %d, got %d", shunt_register_expected, shunt_register_actual);
}

static void test_current(struct ina230_fixture *fixture)
{
	/* 16-bit signed value for current register */
	const int16_t current_reg_vectors[] = {
		32767,
		1000,
		100,
		1,
		0,
		-1,
		-100,
		-1000,
		-32768,
	};

	ARRAY_FOR_EACH_PTR(current_reg_vectors, current_register) {
		struct sensor_value sensor_val;
		double current_expected_A = fixture->current_lsb_uA * 1e-6 * *current_register;

		/* set current reading */
		ina230_mock_set_register(fixture->mock->data, INA230_REG_CURRENT,
					 *current_register);

		/* Verify sensor value is correct */
		zassert_ok(sensor_sample_fetch(fixture->dev));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_CURRENT, &sensor_val));
		double current_actual_A = sensor_value_to_double(&sensor_val);

		zexpect_within(current_expected_A, current_actual_A, fixture->current_lsb_uA*1e-6,
			"Expected %.6f A, got %.6f A", current_expected_A, current_actual_A);
	}
}

static void test_bus_voltage(struct ina230_fixture *fixture)
{
	zassert_not_null(fixture->mock);

	/* 16-bit signed value for voltage register (but always positive) 1.25 mV/bit */
	const int16_t voltage_reg_vectors[] = {
		32767,
		28800,	/* 36V maximum voltage */
		1000,
		100,
		1,
		0,
	};

	double bitres = fixture->dev_type == INA236 ? 1.6e-3 : 1.25e-3;

	ARRAY_FOR_EACH_PTR(voltage_reg_vectors, voltage_register) {
		struct sensor_value sensor_val;

		ina230_mock_set_register(fixture->mock->data, INA230_REG_BUS_VOLT,
			*voltage_register);

		/* Verify sensor value is correct */
		zassert_ok(sensor_sample_fetch(fixture->dev));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_VOLTAGE, &sensor_val));

		double voltage_actual_V = sensor_value_to_double(&sensor_val);
		double voltage_expected_V = *voltage_register * bitres;

		zexpect_within(voltage_expected_V, voltage_actual_V, 1e-6,
			"Expected %.6f A, got %.6f A", voltage_expected_V, voltage_actual_V);
	}
}

static void test_power(struct ina230_fixture *fixture)
{
	/* 16-bit unsigned value for power register */
	const uint16_t power_reg_vectors[] = {
		65535,
		32767,
		10000,
		1000,
		100,
		1,
		0,
	};

	int scale = fixture->dev_type == INA236 ? 32 : 25;

	ARRAY_FOR_EACH_PTR(power_reg_vectors, power_register) {
		struct sensor_value sensor_val;

		/* power is power_register * SCALE * current_lsb */
		double power_expected_W = *power_register * scale * fixture->current_lsb_uA * 1e-6;

		/* set current reading */
		ina230_mock_set_register(fixture->mock->data, INA230_REG_POWER, *power_register);

		/* Verify sensor value is correct */
		zassert_ok(sensor_sample_fetch(fixture->dev));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_POWER, &sensor_val));
		double power_actual_W = sensor_value_to_double(&sensor_val);

		zexpect_within(power_expected_W, power_actual_W, 1e-6,
			       "Expected %.6f W, got %.6f W for %d", power_expected_W,
			       power_actual_W, *power_register);
	}
}

static void test_shunt_voltage(struct ina230_fixture *fixture)
{
	/* 16-bit signed value for vshunt register */
	const int16_t vshunt_reg_vectors[] = {
		32767, 1000, 100, 1, 0, -1, -100, -1000, -32768,
	};

	ARRAY_FOR_EACH_PTR(vshunt_reg_vectors, vshunt_register) {
		struct sensor_value sensor_val;

		/* shunt voltage is vshunt_register * 2.5 uV */
		double vshunt_expected_mV = *vshunt_register * 2.5 * 1e-3;

		/* set current reading */
		ina230_mock_set_register(fixture->mock->data, INA230_REG_SHUNT_VOLT,
					 *vshunt_register);

		/* Verify sensor value is correct */
		zassert_ok(sensor_sample_fetch(fixture->dev));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_VSHUNT, &sensor_val));
		double vshunt_actual_mV = sensor_value_to_double(&sensor_val);

		zexpect_within(vshunt_expected_mV, vshunt_actual_mV, 1e-6,
			       "Expected %.6f mV, got %.6f mV for %d", vshunt_expected_mV,
			       vshunt_actual_mV, *vshunt_register);
	}
}

FAKE_VOID_FUNC(test_cnvr_trigger_handler, const struct device *, const struct sensor_trigger *);
FAKE_VOID_FUNC(test_alert_trigger_handler, const struct device *, const struct sensor_trigger *);

static void test_trigger(struct ina230_fixture *fixture)
{
	const struct sensor_trigger trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	const struct sensor_trigger alarm_trigger = {
		.type = (enum sensor_trigger_type)SENSOR_TRIG_INA230_OVER,
		.chan = SENSOR_CHAN_VOLTAGE,
	};

	int ret;

	RESET_FAKE(test_cnvr_trigger_handler);
	RESET_FAKE(test_alert_trigger_handler);

	ret = sensor_trigger_set(fixture->dev, &trigger, test_cnvr_trigger_handler);
	zassert_equal(ret, 0);

	ret = sensor_trigger_set(fixture->dev, &alarm_trigger, test_alert_trigger_handler);
	zassert_equal(ret, 0);

	ina230_mock_set_register(fixture->mock->data, INA230_REG_MASK,
				 INA230_CONVERSION_READY | INA230_CONVERSION_READY_FLAG |
					 INA230_ALERT_FUNCTION_FLAG | INA230_BUS_VOLTAGE_OVER);

	/* Toggle the GPIO */
	gpio_emul_input_set(fixture->alert_gpios.port, fixture->alert_gpios.pin, 1);
	k_msleep(5);
	gpio_emul_input_set(fixture->alert_gpios.port, fixture->alert_gpios.pin, 0);
	k_msleep(5);

	/* Verify the handler was called */
	zassert_equal(test_cnvr_trigger_handler_fake.call_count, 1);
	zassert_equal(test_alert_trigger_handler_fake.call_count, 1);
}

static void test_trigger_config(struct ina230_fixture *fixture)
{
	const struct sensor_trigger cnvr_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	struct sensor_trigger alarm_trigger = {
		.type = (enum sensor_trigger_type)SENSOR_TRIG_INA230_OVER,
		.chan = SENSOR_CHAN_VOLTAGE,
	};
	struct sensor_value reg;
	int ret;

	/* Remove trigger */
	ret = sensor_trigger_set(fixture->dev, &cnvr_trigger, NULL);
	zassert_equal(ret, 0);

	ret = sensor_trigger_set(fixture->dev, &alarm_trigger, NULL);
	zassert_equal(ret, 0);

	sensor_attr_get(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_FEATURE_MASK, &reg);
	zassert_equal(reg.val1, 0);

	/* Add triggers */
	ret = sensor_trigger_set(fixture->dev, &cnvr_trigger, test_cnvr_trigger_handler);
	zassert_equal(ret, 0);

	sensor_attr_get(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_FEATURE_MASK, &reg);
	zassert_equal(reg.val1 & INA230_CONVERSION_READY, INA230_CONVERSION_READY);

	ret = sensor_trigger_set(fixture->dev, &alarm_trigger, test_alert_trigger_handler);
	zassert_equal(ret, 0);

	sensor_attr_get(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_FEATURE_MASK, &reg);
	zassert_equal(reg.val1 & (INA230_CONVERSION_READY | INA230_BUS_VOLTAGE_OVER),
		      (INA230_CONVERSION_READY | INA230_BUS_VOLTAGE_OVER));

	/* Update trigger */
	alarm_trigger.type = (enum sensor_trigger_type)SENSOR_TRIG_INA230_UNDER;
	alarm_trigger.chan = SENSOR_CHAN_VSHUNT;

	ret = sensor_trigger_set(fixture->dev, &alarm_trigger, test_alert_trigger_handler);
	zassert_equal(ret, 0);

	sensor_attr_get(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_FEATURE_MASK, &reg);
	zassert_equal(reg.val1 & (INA230_CONVERSION_READY | INA230_SHUNT_VOLTAGE_UNDER),
		      (INA230_CONVERSION_READY | INA230_SHUNT_VOLTAGE_UNDER));
}

/* Create a test fixture for each enabled ina230 device node */
#define INA230_FIXTURE_ENTRY(inst, v)                                                              \
	static struct ina230_fixture fixture_23##v##_##inst = {                                    \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
		.mock = EMUL_DT_GET(DT_DRV_INST(inst)),                                            \
		.current_lsb_uA = DT_INST_PROP(inst, current_lsb_microamps),                       \
		.rshunt_uOhms = DT_INST_PROP(inst, rshunt_micro_ohms),                             \
		.dev_type = INA23##v,                                                              \
		.alert_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),                   \
	}

/* Create a test suite for each enabled ina230 device node */
#define INA230_TESTS(inst, v)                                                                      \
	INA230_FIXTURE_ENTRY(inst, v);                                                             \
	ZTEST(ina23##v##_##inst, test_datasheet_example)                                           \
	{                                                                                          \
		test_datasheet_example(&fixture_23##v##_##inst);                                   \
	}                                                                                          \
	ZTEST(ina23##v##_##inst, test_shunt_cal)                                                   \
	{                                                                                          \
		test_shunt_cal(&fixture_23##v##_##inst);                                           \
	}                                                                                          \
	ZTEST(ina23##v##_##inst, test_current)                                                     \
	{                                                                                          \
		test_current(&fixture_23##v##_##inst);                                             \
	}                                                                                          \
	ZTEST(ina23##v##_##inst, test_bus_voltage)                                                 \
	{                                                                                          \
		test_bus_voltage(&fixture_23##v##_##inst);                                         \
	}                                                                                          \
	ZTEST(ina23##v##_##inst, test_power)                                                       \
	{                                                                                          \
		test_power(&fixture_23##v##_##inst);                                               \
	}                                                                                          \
	ZTEST(ina23##v##_##inst, test_shunt_voltage)                                               \
	{                                                                                          \
		test_shunt_voltage(&fixture_23##v##_##inst);                                       \
	}                                                                                          \
	ZTEST(ina23##v##_##inst, test_trigger)                                                     \
	{                                                                                          \
		test_trigger(&fixture_23##v##_##inst);                                             \
	}                                                                                          \
	ZTEST(ina23##v##_##inst, test_trigger_config)                                              \
	{                                                                                          \
		test_trigger_config(&fixture_23##v##_##inst);                                      \
	}                                                                                          \
	ZTEST_SUITE(ina23##v##_##inst, NULL, NULL, NULL, NULL, NULL);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ina230
DT_INST_FOREACH_STATUS_OKAY_VARGS(INA230_TESTS, 0)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ina236
DT_INST_FOREACH_STATUS_OKAY_VARGS(INA230_TESTS, 6)
