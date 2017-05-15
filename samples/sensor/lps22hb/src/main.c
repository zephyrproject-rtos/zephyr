/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <stdio.h>

void main(void)
{
	struct sensor_value temp, press;
	struct device *dev = device_get_binding("LPS22HB");

	if (dev == NULL) {
		printf("Could not get LPS22HB device\n");
		return;
	}

	while (1) {

		if (sensor_sample_fetch(dev) < 0) {
			printf("Sensor sample update error\n");
			return;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_TEMP, &temp) < 0) {
			printf("Cannot read LPS22HB temperature channel");
			return;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press) < 0) {
			printf("Cannot read LPS22HB pressure channel");
			return;
		}

		printf("Temperature: %d.%02d C; Pressure: %d.%01d mb\n",
		       temp.val1, temp.val2, press.val1, press.val2);

		k_sleep(1000);
	}
}
