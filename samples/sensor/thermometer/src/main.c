/*
 * Copyright (c) 2016 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sensor.h>
#include <stdio.h>

void main(void)
{
	struct device *temp_dev;

	printf("Thermometer Example! %s\n", CONFIG_ARCH);

	temp_dev = device_get_binding("TEMP_0");
	if (!temp_dev) {
		printf("error: no temp device\n");
		return;
	}

	printf("temp device is %p, name is %s\n",
	       temp_dev, temp_dev->config->name);

	while (1) {
		int r;
		struct sensor_value temp_value;

		r = sensor_sample_fetch(temp_dev);
		if (r) {
			printf("sensor_sample_fetch failed return: %d\n", r);
			break;
		}

		r = sensor_channel_get(temp_dev, SENSOR_CHAN_AMBIENT_TEMP,
				       &temp_value);
		if (r) {
			printf("sensor_channel_get failed return: %d\n", r);
			break;
		}

		printf("Temperature is %gC\n",
		       sensor_value_to_double(&temp_value));

		k_sleep(1000);
	}
}
