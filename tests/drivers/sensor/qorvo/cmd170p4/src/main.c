/*
 * Copyright (c) 2026 HawkEye 360
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

/* Tolerance for power comparisons ±0.2 dBm */
#define POWER_DBM_TOLERANCE 0.2

#define TEST_NODE_0 DT_NODELABEL(rf_power_detector)

const struct device *get_sensor_device(void)
{
	const struct device *const sensor_dev = DEVICE_DT_GET(TEST_NODE_0);

	zassert_true(device_is_ready(sensor_dev), "Sensor device is not ready");

	return sensor_dev;
}

/**
 * @brief Initialize ADC channel and set emulated voltage
 */
static int init_adc_with_voltage(int input_mv)
{
	struct adc_dt_spec spec = ADC_DT_SPEC_GET_BY_IDX(TEST_NODE_0, 0);
	int ret;

	zassert_true(adc_is_ready_dt(&spec), "ADC device is not ready");

	ret = adc_channel_setup_dt(&spec);
	zassert_equal(ret, 0, "Setting up ADC channel failed with code %d", ret);

	ret = adc_emul_const_value_set(spec.dev, spec.channel_id, input_mv);
	zassert_ok(ret, "adc_emul_const_value_set() failed with code %d", ret);

	return ret;
}

/**
 * Test basic sensor initialization and read
 */
ZTEST_USER(cmd170p4, test_sensor_init)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val;
	double dbm;
	int ret;

	/* Set ADC to read 1V (18.95 dBm) */
	ret = init_adc_with_voltage(1000);
	zassert_ok(ret, "Failed to initialize ADC");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "sample_fetch should succeed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val);
	zassert_ok(ret, "channel_get should succeed");

	dbm = sensor_value_to_double(&val);
	zassert_within(dbm, 18.95, POWER_DBM_TOLERANCE,
		       "Expected ~18.95 dBm, got %.2f dBm", dbm);
}

/**
 * Test known voltage-to-power conversion: 0V = 0 dBm
 */
ZTEST_USER(cmd170p4, test_voltage_0mv)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val;
	double dbm;
	int ret;

	ret = init_adc_with_voltage(0);
	zassert_ok(ret, "Failed to set ADC voltage");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "sample_fetch failed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val);
	zassert_ok(ret, "channel_get failed");

	dbm = sensor_value_to_double(&val);
	zassert_within(dbm, 0.0, POWER_DBM_TOLERANCE,
		       "Expected ~0.0 dBm, got %.2f dBm", dbm);
}

/**
 * Test known voltage-to-power conversion: 1.105V = 19.43 dBm
 */
ZTEST_USER(cmd170p4, test_voltage_1105mv)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val;
	double dbm;
	int ret;

	ret = init_adc_with_voltage(1105);
	zassert_ok(ret, "Failed to set ADC voltage");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "sample_fetch failed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val);
	zassert_ok(ret, "channel_get failed");

	dbm = sensor_value_to_double(&val);
	zassert_within(dbm, 19.43, POWER_DBM_TOLERANCE,
		       "Expected ~19.43 dBm, got %.2f dBm", dbm);
}

/**
 * Test known voltage-to-power conversion: 2.452V = 23.81 dBm
 */
ZTEST_USER(cmd170p4, test_voltage_2452mv)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val;
	double dbm;
	int ret;

	ret = init_adc_with_voltage(2452);
	zassert_ok(ret, "Failed to set ADC voltage");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "sample_fetch failed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val);
	zassert_ok(ret, "channel_get failed");

	dbm = sensor_value_to_double(&val);
	zassert_within(dbm, 23.81, POWER_DBM_TOLERANCE,
		       "Expected ~23.81 dBm, got %.2f dBm", dbm);
}

/**
 * Test known voltage-to-power conversion: 4.469V = 27.75 dBm
 */
ZTEST_USER(cmd170p4, test_voltage_4469mv)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val;
	double dbm;
	int ret;

	ret = init_adc_with_voltage(4469);
	zassert_ok(ret, "Failed to set ADC voltage");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "sample_fetch failed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val);
	zassert_ok(ret, "channel_get failed");

	dbm = sensor_value_to_double(&val);
	zassert_within(dbm, 27.75, POWER_DBM_TOLERANCE,
		       "Expected ~27.75 dBm, got %.2f dBm", dbm);
}

/**
 * Test interpolation between table points
 * Test 500 mV which is ~15.88 dBm (between table entries)
 */
ZTEST_USER(cmd170p4, test_interpolation)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val;
	double dbm;
	int ret;

	ret = init_adc_with_voltage(500);
	zassert_ok(ret, "Failed to set ADC voltage");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "sample_fetch failed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val);
	zassert_ok(ret, "channel_get failed");

	dbm = sensor_value_to_double(&val);
	zassert_within(dbm, 15.88, POWER_DBM_TOLERANCE,
		       "Expected ~15.88 dBm, got %.2f dBm", dbm);
}

/**
 * Test unsupported channel returns error
 */
ZTEST_USER(cmd170p4, test_unsupported_channel)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val;
	int ret;

	ret = init_adc_with_voltage(1000);
	zassert_ok(ret, "Failed to initialize ADC");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "sample_fetch should succeed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &val);
	zassert_equal(ret, -ENOTSUP, "Unsupported channel should return -ENOTSUP");
}

/**
 * Test multiple consecutive reads with different voltages
 */
ZTEST_USER(cmd170p4, test_multiple_reads)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val1, val2;
	double dbm1, dbm2;
	int ret;

	/* First read at 1105 mV (19.43 dBm) */
	ret = init_adc_with_voltage(1105);
	zassert_ok(ret, "Failed to set first voltage");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "First sample_fetch failed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val1);
	zassert_ok(ret, "First channel_get failed");

	/* Second read at 2452 mV (23.81 dBm) */
	ret = init_adc_with_voltage(2452);
	zassert_ok(ret, "Failed to set second voltage");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "Second sample_fetch failed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val2);
	zassert_ok(ret, "Second channel_get failed");

	/* Verify both readings are valid and different */
	dbm1 = sensor_value_to_double(&val1);
	dbm2 = sensor_value_to_double(&val2);

	zassert_within(dbm1, 19.43, POWER_DBM_TOLERANCE,
		       "First reading should be ~19.43 dBm");
	zassert_within(dbm2, 23.81, POWER_DBM_TOLERANCE,
		       "Second reading should be ~23.81 dBm");
}

/**
 * Test sensor_value format and conversion to double
 */
ZTEST_USER(cmd170p4, test_sensor_value_format)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val;
	double dbm;
	int ret;

	/* Set to 1105 mV (19.43 dBm) */
	ret = init_adc_with_voltage(1105);
	zassert_ok(ret, "Failed to set voltage");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "sample_fetch failed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val);
	zassert_ok(ret, "channel_get failed");

	dbm = sensor_value_to_double(&val);
	zassert_within(dbm, 19.43, POWER_DBM_TOLERANCE,
		       "Expected ~19.43 dBm, got %.2f dBm", dbm);
}

/**
 * Test edge case: maximum detector voltage
 * Note: Physical detector maxes at 5V
 */
ZTEST_USER(cmd170p4, test_max_table_voltage)
{
	const struct device *sensor = get_sensor_device();
	struct sensor_value val;
	double dbm;
	int ret;

	/* Set ADC to 5000 mV (detector max, ~28.56 dBm) */
	ret = init_adc_with_voltage(5000);
	zassert_ok(ret, "Failed to set ADC voltage");

	ret = sensor_sample_fetch(sensor);
	zassert_ok(ret, "sample_fetch failed");

	ret = sensor_channel_get(sensor, SENSOR_CHAN_POWER_DBM, &val);
	zassert_ok(ret, "channel_get failed");

	dbm = sensor_value_to_double(&val);
	zassert_within(dbm, 28.56, POWER_DBM_TOLERANCE,
		       "Expected ~28.56 dBm, got %.2f dBm", dbm);
}

static void *cmd170p4_setup(void)
{
	const struct device *sensor = get_sensor_device();

	k_object_access_grant(sensor, k_current_get());

	return NULL;
}

ZTEST_SUITE(cmd170p4, NULL, cmd170p4_setup, NULL, NULL, NULL);
