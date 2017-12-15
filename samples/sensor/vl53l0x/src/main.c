/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <misc/printk.h>

void main(void)
{
	struct device *dev = device_get_binding(CONFIG_VL53L0X_NAME);
	struct sensor_value prox_value;
	int ret;

	if (dev == NULL) {
		printk("Could not get VL53L0X device\n");
		return;
	}

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &prox_value);
		printk("prox is %d\n", prox_value.val1);

		ret = sensor_channel_get(dev,
					 SENSOR_CHAN_DISTANCE,
					 &prox_value);
		printk("distance is %d\n", prox_value.val1);

		k_sleep(1000);
	}
}
