/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>

void main(void)
{
	const struct device *dev;
	struct sensor_value co2, voc;

	dev = device_get_binding(DT_LABEL(DT_INST(0, ams_iaqcore)));
	if (!dev) {
		printk("Failed to get device binding");
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->name);

	while (1) {
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_CO2, &co2);
		sensor_channel_get(dev, SENSOR_CHAN_VOC, &voc);
		printk("Co2: %d.%06dppm; VOC: %d.%06dppb\n",
			co2.val1, co2.val2,
			voc.val1, voc.val2);

		k_sleep(K_MSEC(1000));
	}
}
