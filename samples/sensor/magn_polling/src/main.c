/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>

static void do_main(const struct device *dev)
{
	int ret;
	struct sensor_value value_x, value_y, value_z;

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &value_x);
		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &value_y);
		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &value_z);
		printf("( x y z ) = ( %f  %f  %f )\n",
		       sensor_value_to_double(&value_x),
		       sensor_value_to_double(&value_y),
		       sensor_value_to_double(&value_z));

		k_sleep(K_MSEC(500));
	}
}

const struct device *sensor_search_for_magnetometer()
{
	static char *magn_sensors[] = {"bmc150_magn", NULL};
	const struct device *dev;
	int i;

	i = 0;
	while (magn_sensors[i]) {
		dev = device_get_binding(magn_sensors[i]);
		if (dev) {
			return dev;
		}
		++i;
	}

	return NULL;
}

void main(void)
{
	const struct device *dev;

	dev = sensor_search_for_magnetometer();
	if (dev) {
		printk("Found device is %p, name is %s\n", dev, dev->name);
		do_main(dev);
	} else {
		printk("There is no available magnetometer device.\n");
	}
}
