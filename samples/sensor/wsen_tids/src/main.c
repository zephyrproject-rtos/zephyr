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
	struct sensor_value temperature;

	if (sensor_sample_fetch(dev) < 0) {
		LOG_ERR("Failed to fetch TIDS sensor sample.");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature) < 0) {
		LOG_ERR("Failed to read TIDS temperature channel.");
		return;
	}

	++sample_count;
	LOG_INF("Sample #%u", sample_count);

	/* Display temperature */
	LOG_INF("Temperature: %.1f C", sensor_value_to_double(&temperature));
}

static void tids_threshold_interrupt_handler(const struct device *dev,
					     const struct sensor_trigger *trig)
{
	LOG_INF("Threshold interrupt triggered.");
}

void main(void)
{
	const struct device *dev = device_get_binding("TIDS");

	if (dev == NULL) {
		LOG_ERR("Failed to get TIDS device.");
		return;
	}

	LOG_INF("TIDS device initialized.");

	if (IS_ENABLED(CONFIG_TIDS_TRIGGER)) {
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_THRESHOLD,
			.chan = SENSOR_CHAN_ALL,
		};
		if (sensor_trigger_set(dev, &trig, tids_threshold_interrupt_handler) < 0) {
			LOG_ERR("Failed to configure trigger.");
			return;
		}
	}

	while (1) {
		process_sample(dev);
		k_sleep(K_MSEC(2000));
	}

	k_sleep(K_FOREVER);
}
