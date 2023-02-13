/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(MAIN);

static void process_sample(const struct device *dev)
{
	static unsigned int sample_count;
	struct sensor_value humidity, temperature;

	if (sensor_sample_fetch(dev) < 0) {
		LOG_ERR("Failed to fetch HIDS sensor sample.");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity) < 0) {
		LOG_ERR("Failed to read HIDS humidity channel.");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature) < 0) {
		LOG_ERR("Failed to read HIDS temperature channel.");
		return;
	}

	++sample_count;
	LOG_INF("Sample #%u", sample_count);

	/* Display humidity */
	LOG_INF("Humidity: %.1f %%", sensor_value_to_double(&humidity));

	/* Display temperature */
	LOG_INF("Temperature: %.1f C", sensor_value_to_double(&temperature));
}

static void hids_drdy_interrupt_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	process_sample(dev);
}

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(we_wsen_hids);

	if (!device_is_ready(dev)) {
		LOG_ERR("sensor: device not ready.\n");
		return;
	}

	LOG_INF("HIDS device initialized.");

	if (IS_ENABLED(CONFIG_WSEN_HIDS_TRIGGER)) {
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ALL,
		};
		if (sensor_trigger_set(dev, &trig, hids_drdy_interrupt_handler) < 0) {
			LOG_ERR("Failed to configure trigger.");
			return;
		}
	}

	while (!IS_ENABLED(CONFIG_WSEN_HIDS_TRIGGER)) {
		process_sample(dev);
		k_msleep(2000);
	}

	k_sleep(K_FOREVER);
}
