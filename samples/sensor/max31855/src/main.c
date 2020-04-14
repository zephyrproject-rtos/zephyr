/*
 * Copyright (c) 2020 Christian Hirsch
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>

void main(void)
{
	struct device *dev;

	dev = device_get_binding(DT_INST_0_MAXIM_MAX31855_LABEL);
	if (dev == NULL) {
		printk("Cannot get device %s\n",
				DT_INST_0_MAXIM_MAX31855_LABEL);
		return;
	}

	printk("dev %p, name %s\n", dev, dev->config->name);
	sensor_sample_fetch(dev);

	while (1) {
		struct sensor_value ambient_temp, die_temp;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
				&ambient_temp);
		sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP,
				&die_temp);

		printk("ambient temp: %d.%06d C; die temp: %d.%06d C\n",
				ambient_temp.val1, ambient_temp.val2,
				die_temp.val1, die_temp.val2);

		k_sleep(K_MSEC(1000));
	}
}
