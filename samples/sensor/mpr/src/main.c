/*
 * Copyright (c) 2020 Sven Herrmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

void main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(honeywell_mpr);
	int rc;

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return;
	}

	while (1) {
		struct sensor_value pressure;

		rc = sensor_sample_fetch(dev);
		if (rc != 0) {
			printf("sensor_sample_fetch error: %d\n", rc);
			break;
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);
		if (rc != 0) {
			printf("sensor_channel_get error: %d\n", rc);
			break;
		}

		printf("pressure: %u.%u kPa\n", pressure.val1, pressure.val2);

		k_sleep(K_SECONDS(1));
	}
}
