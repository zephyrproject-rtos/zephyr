/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <drivers/sensor.h>
#include <stdio.h>

void do_main(struct device *dev)
{
	int ret;
	struct sensor_value x, y, z;

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &x);
		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &y);
		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &z);

		printf("( x y z ) = ( %f  %f  %f )\n",
				sensor_value_to_double(&x),
				sensor_value_to_double(&y),
				sensor_value_to_double(&z));

		k_sleep(K_MSEC(500));
	}
}

struct device *sensor_search()
{
	static const char *const magn_sensor[] = { "bmm150", NULL };
	struct device *dev;
	int i;

	i = 0;
	while (magn_sensor[i]) {
		dev = device_get_binding(magn_sensor[i]);
		if (dev) {
			printk("device binding\n");
			return dev;
		}

		++i;
	}
	return NULL;
}

void main(void)
{
	struct device *dev;

	printk("BMM150 Geomagnetic sensor Application\n");

	dev = sensor_search();
	if (dev) {
		printk("Found device is %p, name is %s\n",
				dev, dev->config->name);
		do_main(dev);
	} else {
		printk("There is no available Geomagnetic device.\n");
	}
}
