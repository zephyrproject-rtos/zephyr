/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/printk.h>
#ifdef CONFIG_SX9500_TRIGGER

static void sensor_trigger_handler(struct device *dev, struct sensor_trigger *trig)
{
	struct sensor_value prox_value;
	int ret;

	ret = sensor_sample_fetch(dev);
	if (ret) {
		printk("sensor_sample_fetch failed ret %d\n", ret);
		return;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &prox_value);
	printk("prox is %d\n", prox_value.val1);
}

static void setup_trigger(struct device *dev)
{
	int ret;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_NEAR_FAR,
	};

	ret = sensor_trigger_set(dev, &trig, sensor_trigger_handler);
	if (ret) {
		printk("sensor_trigger_set err %d\n", ret);
	}
}

void do_main(struct device *dev)
{
	setup_trigger(dev);

	while (1) {
		k_sleep(K_MSEC(1000));
	}
}

#else /* CONFIG_SX9500_TRIGGER */

static void do_main(struct device *dev)
{
	int ret;
	struct sensor_value prox_value;

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &prox_value);
		printk("prox is %d\n", prox_value.val1);

		k_sleep(K_MSEC(1000));
	}
}

#endif /* CONFIG_SX9500_TRIGGER */

void main(void)
{
	struct device *dev;

	dev = device_get_binding("SX9500");

	if (dev == NULL) {
		printk("Could not get SX9500 device\n");
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->config->name);

	do_main(dev);
}
