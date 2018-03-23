/*
 * Copyright (c) 2016 Firmwave
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <misc/printk.h>
#include <misc/__assert.h>

static void do_main(struct device *dev)
{
	int ret;
	struct sensor_value temp_value;
	struct sensor_value attr;

	attr.val1 = 150;
	attr.val2 = 0;
	ret = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
			      SENSOR_ATTR_FULL_SCALE, &attr);
	if (ret) {
		printk("sensor_attr_set failed ret %d\n", ret);
		return;
	}

	attr.val1 = 8;
	attr.val2 = 0;
	ret = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);
	if (ret) {
		printk("sensor_attr_set failed ret %d\n", ret);
		return;
	}

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_value);
		if (ret) {
			printk("sensor_channel_get failed ret %d\n", ret);
			return;
		}

		printk("temp is %d (%d micro)\n", temp_value.val1,
		       temp_value.val2);

		k_sleep(1000);
	}
}

void main(void)
{
	struct device *dev;

	dev = device_get_binding("TMP112");
	__ASSERT(dev != NULL, "Failed to get device binding");
	printk("device is %p, name is %s\n", dev, dev->config->name);

	do_main(dev);
}
