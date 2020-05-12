/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/printk.h>

void main(void)
{
	const struct device *dev = device_get_binding(DT_LABEL(DT_INST(0, st_vl53l0x)));
	struct sensor_value value;
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

		ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &value);
		printk("prox is %d\n", value.val1);

		ret = sensor_channel_get(dev,
					 SENSOR_CHAN_DISTANCE,
					 &value);
		printf("distance is %.3fm\n", sensor_value_to_double(&value));

		k_sleep(K_MSEC(1000));
	}
}
