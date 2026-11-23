/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <stdio.h>
#include <zephyr/drivers/sensor.h>

#define SLEEP_TIME K_MSEC(1000)

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(light_sensor));

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	while (1) {
		int read;
		struct sensor_value lux;

		read = sensor_sample_fetch(dev);
		if (read) {
			printf("sample fetch error %d\n", read);
			continue;
		}

		sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &lux);

		printf("lux: %f\n", sensor_value_to_double(&lux));

		k_sleep(SLEEP_TIME);
	}
	return 0;
}
