/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>

LOG_MODULE_REGISTER(MAIN);

static void process_sample(const struct device *dev)
{
	static unsigned int sample_count;
	struct sensor_value pressure, temperature;

	if (sensor_sample_fetch(dev) < 0) {
		LOG_ERR("Failed to fetch PDUS sensor sample.");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure) < 0) {
		LOG_ERR("Failed to read PDUS pressure channel.");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature) < 0) {
		LOG_ERR("Failed to read PDUS temperature channel.");
		return;
	}

	++sample_count;
	LOG_INF("Sample #%u", sample_count);

	/* Display pressure */
	LOG_INF("Pressure: %.4f kPa", sensor_value_to_double(&pressure));

	/* Display temperature */
	LOG_INF("Temperature: %.1f C", sensor_value_to_double(&temperature));
}

void main(void)
{
	const struct device *dev = device_get_binding("PDUS");

	if (dev == NULL) {
		LOG_ERR("Failed to get PDUS device.");
		return;
	}

	LOG_INF("PDUS device initialized.");

	while (1) {
		process_sample(dev);
		k_sleep(K_MSEC(2000));
	}

	k_sleep(K_FOREVER);
}
