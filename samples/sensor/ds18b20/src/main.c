/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
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
	struct sensor_value temp;
	struct device *dev = device_get_binding(CONFIG_DS18B20_NAME);

	if (dev == NULL) {
		printf("Could not get DS18B20 device\n");
		return;
	}

	while (1) {
		if (sensor_sample_fetch(dev) < 0) {
			printf("Sensor sample update error\n");
			return;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
			printf("Cannot read DS18B20 temperature channel\n");
			return;
		}

		/* display temperature */
		printf("Temperature: %.2f C\n", sensor_value_to_double(&temp));

		k_sleep(2000);
	}
}
