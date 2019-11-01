/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/sensor.h>

/**
 * @file Sample app using the MAX44009 light sensor through ARC I2C.
 *
 * This app will read the sensor reading via I2C interface and show
 * the result every 4 seconds.
 */

void main(void)
{
	struct device *dev;
	struct sensor_value val;
	u32_t lum = 0U;

	printk("MAX44009 light sensor application\n");

	dev = device_get_binding("MAX44009");
	if (!dev) {
		printk("sensor: device not found.\n");
		return;
	}

	while (1) {
		if (sensor_sample_fetch_chan(dev, SENSOR_CHAN_LIGHT) != 0) {
			printk("sensor: sample fetch fail.\n");
			return;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &val) != 0) {
			printk("sensor: channel get fail.\n");
			return;
		}

		lum = val.val1;
		printk("sensor: lum reading: %d\n", lum);

		k_sleep(K_MSEC(4000));
	}
}
