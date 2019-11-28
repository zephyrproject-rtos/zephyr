/*
 * Copyright (c) 2018 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>

static void do_main(struct device *dev)
{
	struct sensor_value co2, voc, voltage, current;

	while (1) {
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_CO2, &co2);
		sensor_channel_get(dev, SENSOR_CHAN_VOC, &voc);
		sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &voltage);
		sensor_channel_get(dev, SENSOR_CHAN_CURRENT, &current);

		printk("Co2: %d.%06dppm; VOC: %d.%06dppb\n", co2.val1, co2.val2,
				voc.val1, voc.val2);
		printk("Voltage: %d.%06dV; Current: %d.%06dA\n\n", voltage.val1,
				voltage.val2, current.val1, current.val2);

		k_sleep(K_MSEC(1000));
	}

}

void main(void)
{
	struct device *dev;

	dev = device_get_binding(DT_INST_0_AMS_CCS811_LABEL);
	if (!dev) {
		printk("Failed to get device binding");
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->config->name);

	do_main(dev);
}
