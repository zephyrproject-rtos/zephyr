/*
 * Copyright (c) 2018 Lars Knudsen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sensor.h>
#include <stdio.h>

void main(void)
{
	struct device *dev = device_get_binding(CONFIG_MMA8451Q_NAME);

	if (dev == NULL) {
		printf("Could not get MMA8451Q device\n");
		return;
	}

	int err;
	struct sensor_value x, y, z;

	printf("XYZ Sensor readings:\n");

	while (1) {
		err = sensor_sample_fetch(dev);
		if (err) {
			printk("Failed to fetch sample for device %s (%d)\n",
					"MMA8451Q", err);
		} else {
			sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &x);
			sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &y);
			sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &z);

			printf("X: %10.6f, Y: %10.6f, Z: %10.6f\n",
				sensor_value_to_double(&x),
				sensor_value_to_double(&y),
				sensor_value_to_double(&z));
		}
		k_sleep(500);
	}
}
