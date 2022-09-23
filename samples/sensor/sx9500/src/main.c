/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/printk.h>
#ifdef CONFIG_SX9500_TRIGGER

static void sensor_trigger_handler(const struct device *dev,
				   const struct sensor_trigger *trig)
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

static void setup_trigger(const struct device *dev)
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

void do_main(const struct device *dev)
{
	setup_trigger(dev);

	while (1) {
		k_sleep(K_MSEC(1000));
	}
}

#else /* CONFIG_SX9500_TRIGGER */

static void do_main(const struct device *dev)
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
	const struct device *const dev = DEVICE_DT_GET_ONE(semtech_sx9500);

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->name);

	do_main(dev);
}
