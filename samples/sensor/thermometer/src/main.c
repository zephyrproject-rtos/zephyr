/*
 * Copyright (c) 2016 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

void main(void)
{
	const struct device *const temp_dev = DEVICE_DT_GET(DT_ALIAS(ambient_temp0));

	printf("Thermometer Example! %s\n", CONFIG_ARCH);

	printf("temp device is %p, name is %s\n",
	       temp_dev, temp_dev->name);

	if (!device_is_ready(temp_dev)) {
		printf("temp device not ready.\n");
		return;
	}

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

		k_sleep(K_MSEC(1000));
	}
}
