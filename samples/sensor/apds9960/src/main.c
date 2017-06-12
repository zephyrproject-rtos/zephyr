/*
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sensor.h>
#include <device.h>
#include <stdio.h>
#include <misc/printk.h>

static void do_main(struct device *dev)
{
	struct sensor_value intensity, pdata;

	while (1) {

		if (sensor_sample_fetch(dev)) {
			printk("sensor_sample fetch failed\n");
		}

		sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &intensity);

		printk("ambient light intensity without"
				" trigger is %d\n", intensity.val1);

		k_sleep(1000);

		sensor_channel_get(dev, SENSOR_CHAN_PROX, &pdata);

		printk("proxy without trigger is %d\n", pdata.val1);

		k_sleep(1000);
	}
}

void main(void)
{
	struct device *dev;

	printk("APDS9960 sample application\n");
	dev = device_get_binding("APDS9960");

	if (!dev) {
		printk("sensor: device not found.\n");
		return;
	}

	do_main(dev);

}
