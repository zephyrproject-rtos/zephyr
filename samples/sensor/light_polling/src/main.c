/*
 * Copyright (c) 2022, Luke Holt
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

void main(void)
{
	/* Light sensor with alias light0 in devicetree. */
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(light0));
	struct sensor_value light;
	int rc;

	if (dev == NULL) {
		/* Device not found, or does not have OK status. */
		printk("Sensor: device not found.\n");
	}

	if (!device_is_ready(dev)) {
		printk("Sensor: device not ready.\n");
	}

	printk("Polling light level data from %s.\n", dev->name);

	while (1) {
		rc = sensor_sample_fetch(dev);
		if (rc != 0) {
			printf("sensor_sample_fetch error: %d\n", rc);
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &light);
		if (rc != 0) {
			printf("sensor_channel_get error: %d\n", rc);
		} else {
			printf("Light: \t%f lux\n", sensor_value_to_double(&light));
		}

		k_sleep(K_MSEC(2000));
	}
}
