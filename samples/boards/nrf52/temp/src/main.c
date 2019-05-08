/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <misc/printk.h>

void main(void)
{
	struct sensor_value temp;

	struct device *dev = device_get_binding(CONFIG_TEMP_NRF5_NAME);
	if (dev == NULL) {
		printk("Could not get TEMP_NRF5 device\n");
		return;
	}

	while (1) {
		if (sensor_sample_fetch(dev) < 0) {
			printk("Sensor sample update error\n");
			return;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temp) < 0) {
			printk("Cannot read TEMP_NRF5 temperature channel\n");
			return;
		}

		/* display temperature */
		printk("Temperature:%d.%06d C\n", temp.val1, temp.val2);
		k_sleep(1000);
	}
}
