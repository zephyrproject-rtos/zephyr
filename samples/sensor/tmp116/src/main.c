/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>
#include <sys/__assert.h>

void main(void)
{
	const struct device *dev;
	struct sensor_value temp_value;
	int ret;

	dev = device_get_binding(DT_LABEL(DT_INST(0, ti_tmp116)));
	__ASSERT(dev != NULL, "Failed to get TMP116 device binding");

	printk("Device %s - %p is ready\n", dev->name, dev);

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("Failed to fetch measurements (%d)\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
					 &temp_value);
		if (ret) {
			printk("Failed to get measurements (%d)\n", ret);
			return;
		}

		printk("temp is %d.%d oC\n", temp_value.val1, temp_value.val2);

		k_sleep(K_MSEC(1000));
	}
}
