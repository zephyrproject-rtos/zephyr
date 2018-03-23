/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <stdio.h>
#include <misc/util.h>

void main(void)
{
	struct sensor_value temp, hum;
	struct device *dev = device_get_binding("HTS221");

	if (dev == NULL) {
		printf("Could not get HTS221 device\n");
		return;
	}

	while (1) {
		if (sensor_sample_fetch(dev) < 0) {
			printf("Sensor sample update error\n");
			return;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
			printf("Cannot read HTS221 temperature channel\n");
			return;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
			printf("Cannot read HTS221 humidity channel\n");
			return;
		}

		/* display temperature */
		printf("Temperature:%.1f C\n", sensor_value_to_double(&temp));

		/* display humidity */
		printf("Relative Humidity:%.1f%%\n",
		       sensor_value_to_double(&hum));

		k_sleep(2000);
	}
}
