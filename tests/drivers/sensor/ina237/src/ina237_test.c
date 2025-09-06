/*
 * Copyright (c) 2023 North River Systems Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/ina237.h>
#include <zephyr/ztest.h>

#include <ina237_emul.h>
#include <ina237.h>

struct ina237_fixture {
	const struct device *dev;
	const struct emul *mock;
	const uint16_t current_lsb_uA;
	const uint16_t rshunt_uOhms;
	const uint16_t config;
};

/**
 * @brief Verify devicetree default configuration is correct.
 */
ZTEST(ina237_0, test_default_config)
{
	const struct ina2xx_config *config;
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ina237_default_test));

	zassert_not_null(dev);
	config = dev->config;
	zassert_not_null(config);

	/* confirm default DT settings */
	zexpect_equal(0xFB68, config->adc_config,
		"0xFB68 != adc_config (0x%x)", config->adc_config);
	zexpect_equal(0x0000, config->config);
}

/**
 * @brief Verify bus voltages for all ina237 nodes in DT
 *
 * @param fixture
 */

static void test_shunt_cal(struct ina237_fixture *fixture)
{
	/* Confirm SHUNT_CAL register which is 819.2e6 * Current_LSB * Rshunt */
	double shunt_cal = 819.2e6 * fixture->current_lsb_uA * 1e-6 * fixture->rshunt_uOhms * 1e-6;

	if (fixture->config & INA237_CFG_HIGH_PRECISION) {
		/* High precision mode */
		shunt_cal *= 4;
	}

	uint32_t shunt_register_actual;
	uint16_t shunt_register_expected = (uint16_t)shunt_cal;

	zassert_ok(ina237_mock_get_register(fixture->mock->data, INA237_REG_CALIB,
		&shunt_register_actual));
	zexpect_within(shunt_register_expected, shunt_register_actual, 1,
		"Expected %d, got %d", shunt_register_expected, shunt_register_actual);
}

static void test_current(struct ina237_fixture *fixture)
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

	for (int idx = 0; idx < ARRAY_SIZE(current_reg_vectors); idx++) {
		struct sensor_value sensor_val;
		int16_t current_register = current_reg_vectors[idx];
		double current_expected_A = fixture->current_lsb_uA * 1e-6 * current_register;

		/* set current reading */
		ina237_mock_set_register(fixture->mock->data, INA237_REG_CURRENT, current_register);

		/* Verify sensor value is correct */
		zassert_ok(sensor_sample_fetch(fixture->dev));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_CURRENT, &sensor_val));
		double current_actual_A = sensor_value_to_double(&sensor_val);

		zexpect_within(current_expected_A, current_actual_A, fixture->current_lsb_uA*1e-6,
			"Expected %.6f A, got %.6f A", current_expected_A, current_actual_A);
	}
}

static void test_bus_voltage(struct ina237_fixture *fixture)
{
	zassert_not_null(fixture->mock);

	/* 16-bit signed value for voltage register (but always positive) 3.125 mV/bit */
	const int16_t voltage_reg_vectors[] = {
		32767,
		27200,	/* 85V maximum voltage */
		1000,
		100,
		1,
		0,
	};

	for (int idx = 0; idx < ARRAY_SIZE(voltage_reg_vectors); idx++) {
		struct sensor_value sensor_val;

		ina237_mock_set_register(fixture->mock->data, INA237_REG_BUS_VOLT,
			voltage_reg_vectors[idx]);

		/* Verify sensor value is correct */
		zassert_ok(sensor_sample_fetch(fixture->dev));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_VOLTAGE, &sensor_val));

		double voltage_actual_V = sensor_value_to_double(&sensor_val);
		double voltage_expected_V = voltage_reg_vectors[idx] * 3.125e-3;

		zexpect_within(voltage_expected_V, voltage_actual_V, 1e-6,
			"Expected %.6f A, got %.6f A", voltage_expected_V, voltage_actual_V);
	}
}

static void test_power(struct ina237_fixture *fixture)
{
	/* 24-bit unsigned value for power register */
	const uint32_t power_reg_vectors[] = {
		16777215,
		65535,
		32767,
		1000,
		100,
		1,
		0,
	};

	for (int idx = 0; idx < ARRAY_SIZE(power_reg_vectors); idx++) {
		struct sensor_value sensor_val;
		uint32_t power_register = power_reg_vectors[idx];

		/* power is 0.2 * current_lsb * register */
		double power_expected_W = 0.2 * fixture->current_lsb_uA * 1e-6 * power_register;

		/* set current reading */
		ina237_mock_set_register(fixture->mock->data, INA237_REG_POWER, power_register);

		/* Verify sensor value is correct */
		zassert_ok(sensor_sample_fetch(fixture->dev));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_POWER, &sensor_val));
		double power_actual_W = sensor_value_to_double(&sensor_val);

		zexpect_within(power_expected_W, power_actual_W, 1e-6,
			"Expected %.6f C, got %.6f C", power_expected_W, power_actual_W);
	}
}

static void test_temperature(struct ina237_fixture *fixture)
{
	zassert_not_null(fixture->mock);

	/* 12-bit signed value with bottom 4-bits reserved - 125 mDegC / bit */
	const int16_t temp_reg_vectors[] = {
		16000,	/* 125C */
		1000,
		100,
		1,
		0,
		-100,
		-1000,
		-5120,	/* -40C */
	};

	for (int idx = 0; idx < ARRAY_SIZE(temp_reg_vectors); idx++) {
		struct sensor_value sensor_val;

		ina237_mock_set_register(fixture->mock->data, INA237_REG_DIETEMP,
		temp_reg_vectors[idx]);

		/* Verify sensor value is correct */
		zassert_ok(sensor_sample_fetch(fixture->dev));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_DIE_TEMP, &sensor_val));

		double temp_actual_degC = sensor_value_to_double(&sensor_val);
		double temp_expected_degC = (temp_reg_vectors[idx] / 16) * 125e-3;

		zexpect_within(temp_expected_degC, temp_actual_degC, 125e-3,
			"Expected %.6f A, got %.6f A", temp_expected_degC, temp_actual_degC);
	}
}

static void test_vshunt(struct ina237_fixture *fixture)
{
	zassert_not_null(fixture->mock);

	/* 16-bit signed value for voltage register (but always positive) 3.125 mV/bit */
	const int16_t vshunt_reg_vectors[] = {
		32767,	/* maximum shunt voltage of 163.84 mV or 40.96 mV */
		27200,
		1000,
		100,
		1,
		0,
		-1,
		-100,
		-1000,
		-32768,	/* minimum shunt voltage of -163.84 mV or -40.96 mV */
	};

	for (int idx = 0; idx < ARRAY_SIZE(vshunt_reg_vectors); idx++) {
		struct sensor_value sensor_val;

		ina237_mock_set_register(fixture->mock->data, INA237_REG_SHUNT_VOLT,
			vshunt_reg_vectors[idx]);

		/* Verify sensor value is correct */
		zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_VSHUNT));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_VSHUNT, &sensor_val));

		double vshunt_actual_mV = sensor_value_to_double(&sensor_val);
		double vshunt_expected_mV = vshunt_reg_vectors[idx];

		if (fixture->config & INA237_CFG_HIGH_PRECISION) {
			/* High precision mode - 1.25 uV/bit = 1250 nV/bit*/
			vshunt_expected_mV *= 1000 * 1.250e-6;
		} else {
			/* Standard precision mode - 5 uV/bit = 5000 nV/bit */
			vshunt_expected_mV *= 1000 * 5e-6;
		}

		zexpect_within(vshunt_expected_mV, vshunt_actual_mV, 1e-9,
			"For %d, Expected %.6f mV, got %.6f mV", vshunt_reg_vectors[idx],
			vshunt_expected_mV, vshunt_actual_mV);
	}
}

/* Create a test fixture for each enabled ina237 device node */
#define DT_DRV_COMPAT ti_ina237
#define INA237_FIXTURE_ENTRY(inst) \
	{ \
		.dev = DEVICE_DT_INST_GET(inst), \
		.mock = EMUL_DT_GET(DT_DRV_INST(inst)), \
		.current_lsb_uA = DT_INST_PROP(inst, current_lsb_microamps), \
		.rshunt_uOhms = DT_INST_PROP(inst, rshunt_micro_ohms), \
		.config = INA237_DT_CONFIG(inst), \
	},

static struct ina237_fixture fixtures[] = {
	DT_INST_FOREACH_STATUS_OKAY(INA237_FIXTURE_ENTRY)
};

/* Create a test suite for each enabled ina237 device node */
#define INA237_TESTS(inst) \
	ZTEST(ina237_##inst, test_shunt_cal) { test_shunt_cal(&fixtures[inst]); } \
	ZTEST(ina237_##inst, test_current) { test_current(&fixtures[inst]); } \
	ZTEST(ina237_##inst, test_bus_voltage) { test_bus_voltage(&fixtures[inst]); } \
	ZTEST(ina237_##inst, test_power) { test_power(&fixtures[inst]); } \
	ZTEST(ina237_##inst, test_temperature) { test_temperature(&fixtures[inst]); } \
	ZTEST(ina237_##inst, test_vshunt) { test_vshunt(&fixtures[inst]); } \
	ZTEST_SUITE(ina237_##inst, NULL, NULL, NULL, NULL, NULL);

DT_INST_FOREACH_STATUS_OKAY(INA237_TESTS)
