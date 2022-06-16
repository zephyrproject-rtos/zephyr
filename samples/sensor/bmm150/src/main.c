/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(bosch_bmm150);
	struct sensor_value x, y, z;
	int ret;

	printk("BMM150 Geomagnetic sensor Application\n");

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return;
	}

	printk("Found device is %p, name is %s\n", dev, dev->name);

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &x);
		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_Y, &y);
		ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_Z, &z);

		printf("( x y z ) = ( %f  %f  %f )\n", sensor_value_to_double(&x),
		       sensor_value_to_double(&y), sensor_value_to_double(&z));

		k_sleep(K_MSEC(500));
	}
}
