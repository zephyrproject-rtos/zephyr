/*
 * Copyright (c) 2024 Yishai Jaffe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

static const struct device *const sensor = DEVICE_DT_GET(DT_ALIAS(si_sensor));

int main(void)
{
	int rc;
	struct sensor_value val;

	if (!device_is_ready(sensor)) {
		printk("sensor: device %s not ready.\n", sensor->name);
		return 0;
	}

	while (1) {
		rc = sensor_sample_fetch(sensor);
		if (rc) {
			printk("Failed to fetch sample (%d)\n", rc);
			return rc;
		}

		rc = sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &val);
		if (rc) {
			printk("Failed to get temperature data (%d)\n", rc);
			return rc;
		}

		printk("Temperature: %.1f °C\n", sensor_value_to_double(&val));

		rc = sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &val);
		if (rc) {
			printk("Failed to get humidity data (%d)\n", rc);
			return rc;
		}

		printk("Humidity: %.1f%%\n", sensor_value_to_double(&val));

		k_sleep(K_SECONDS(1));
	}
}
