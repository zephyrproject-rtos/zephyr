/*
 * Copyright (c) 2020, Philémon Jaermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/sensor.h>
#include <stdio.h>

void main(void)
{
	struct device *dev =
		device_get_binding(DT_INST_0_SENSIRION_SHT21_LABEL);
	struct sensor_value humidity;
	struct sensor_value temp;

	if (dev == NULL) {
		printf("Could not get %s device\n",
				DT_INST_0_SENSIRION_SHT21_LABEL);
		return;
	}

	while (1) {
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);

		/* Print humidity/temperature data */
		printf("Humidity (%%RH) = %d.%d\n",
				humidity.val1, humidity.val2);
		printf("Temperature (Celcius) = %d.%d\n\n",
				temp.val1, temp.val2);

		k_sleep(K_SECONDS(1));
	}
}
