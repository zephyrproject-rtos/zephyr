/*
 * Copyright (c) 2020 George Gkinis
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <device.h>
#include <drivers/sensor.h>
#include <drivers/sensor/hx711.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

const struct device *hx711;

void reset(void)
{
	LOG_INF("Setting HX711_POWER_OFF");
	avia_hx711_power(hx711, HX711_POWER_OFF);
	k_msleep(1000);
	LOG_INF("Setting HX711_POWER_ON");
	avia_hx711_power(hx711, HX711_POWER_ON);
}

void set_rate(enum hx711_rate rate)
{
	static struct sensor_value rateVal;

	rateVal.val1 = rate;
	sensor_attr_set(hx711,
			SENSOR_CHAN_WEIGHT,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&rateVal);
}

void measure(void)
{
	static struct sensor_value weight;
	int ret;

	ret = sensor_sample_fetch(hx711);
	if (ret != 0) {
		LOG_ERR("Cannot take measurement: %d  ", ret);
	} else {
		sensor_channel_get(hx711, SENSOR_CHAN_WEIGHT, &weight);
		LOG_INF("weight: %d\n", weight.val1);
	}
}

void main(void)
{
	hx711 = device_get_binding("HX711");

	__ASSERT(hx711, "Failed to get device binding");

	LOG_INF("device is %p, name is %s\n", hx711, hx711->name);

	LOG_INF("Offset set to %d\n", avia_hx711_tare(hx711, 5));

	LOG_INF("waiting for known weight of %d grams...",
		CONFIG_HX711_CALIBRATION_WEIGHT);

	for (int i = 5; i >= 0; i--) {
		LOG_INF(" %d..", i);
		k_msleep(1000);
	}

	LOG_INF("Calculating slope...\n");
	avia_hx711_calibrate(hx711, CONFIG_HX711_CALIBRATION_WEIGHT, 5);

	while (true) {
		k_msleep(1000);
		LOG_INF("Test measure");
		LOG_INF("Setting sampling rate to 10Hz.");
		set_rate(HX711_RATE_10HZ);
		measure();

		k_msleep(1000);
		LOG_INF("Setting sampling rate to 80Hz.");
		set_rate(HX711_RATE_80HZ);
		measure();

		k_msleep(1000);
		LOG_INF("Test measure directly after reset");
		LOG_INF("Setting sampling rate to 10Hz.");
		set_rate(HX711_RATE_10HZ);
		reset();
		measure();

		k_msleep(1000);
		LOG_INF("Setting sampling rate to 80Hz.");
		set_rate(HX711_RATE_80HZ);
		reset();
		measure();

		k_msleep(1000);
		LOG_INF("Test measure during POWER_OFF");
		LOG_INF("Setting sampling rate to 10Hz.");
		set_rate(HX711_RATE_10HZ);
		LOG_INF("Setting HX711_POWER_OFF");
		avia_hx711_power(hx711, HX711_POWER_OFF);
		measure();
		LOG_INF("Setting sampling rate to 80Hz.");
		set_rate(HX711_RATE_80HZ);
		measure();
		k_msleep(10000);
	}
}
