/*
 * Copyright (c) 2016 Firmwave
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>

static void do_main(const struct device *dev)
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

		k_sleep(K_MSEC(1000));
	}
}

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(ti_tmp112);

	__ASSERT(dev != NULL, "Failed to get device binding");
	__ASSERT(device_is_ready(dev), "Device %s is not ready", dev->name);
	printk("device is %p, name is %s\n", dev, dev->name);

	do_main(dev);
}
