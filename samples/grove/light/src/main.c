/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <misc/printk.h>
#include <stdio.h>
#include <sensor.h>

#define SLEEP_TIME	1000

void main(void)
{
	struct device *dev = device_get_binding("GROVE_LIGHT_SENSOR");

	if (dev == NULL) {
		printk("device not found.  aborting test.\n");
		return;
	}
	while (1) {
		struct sensor_value lux;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &lux);

		printf("lux: %f\n", sensor_value_to_double(&lux));

		k_sleep(SLEEP_TIME);
	}
}
