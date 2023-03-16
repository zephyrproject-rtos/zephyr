/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2023 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

void main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ambient_temp0));
	struct sensor_value value;
	int ret;

	printf("Thermometer Example (%s)\n", CONFIG_ARCH);

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return;
	}

	printf("Temperature device is %p, name is %s\n", dev, dev->name);

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret != 0) {
			printf("Sensor fetch failed: %d\n", ret);
			break;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &value);
		if (ret != 0) {
			printf("Sensor get failed: %d\n", ret);
			break;
		}

		printf("Temperature is %0.1lf°C\n", sensor_value_to_double(&value));

		k_sleep(K_MSEC(1000));
	}
}
