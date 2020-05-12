/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <stdio.h>
#include <drivers/sensor.h>

#define SLEEP_TIME	K_MSEC(1000)

void main(void)
{
	const struct device *dev = device_get_binding(DT_LABEL(DT_INST(0, grove_light)));

	if (dev == NULL) {
		printf("device not found.  aborting test.\n");
		return;
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
}
