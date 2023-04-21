/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define LO_CELSIUS (15)
#define HI_CELSIUS (30)

void test_thermistor_read(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(infineon_thermistor);
	struct sensor_value value;
	char invalid_temp_msg[40];
	double temp_celsius;

	zassert_true(device_is_ready(dev), "Thermistor is not ready");

	zassert_ok(sensor_sample_fetch(dev), "Failed to fetch sample");
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &value),
		   "Failed to get sensor data");

	temp_celsius = sensor_value_to_double(&value);
	sprintf(invalid_temp_msg, "Invalid temperature: %.2f C", temp_celsius);
	zassert_true((temp_celsius > LO_CELSIUS) && (temp_celsius < HI_CELSIUS), invalid_temp_msg);
}

void test_thermistor_bad_chan(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(infineon_thermistor);
	struct sensor_value value;

	zassert_true(device_is_ready(dev), "Thermistor is not ready");

	zassert_ok(sensor_sample_fetch(dev), "Failed to fetch sample");
	zassert_true(sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &value) != 0,
		     "Invalid sensor channel accepted");
}

ZTEST(thermistor, test_thermistor_read) {
	test_thermistor_read();
}

ZTEST(thermistor, test_thermistor_bad_chan) {
	test_thermistor_bad_chan();
}

ZTEST_SUITE(thermistor, NULL, NULL, NULL, NULL, NULL);
