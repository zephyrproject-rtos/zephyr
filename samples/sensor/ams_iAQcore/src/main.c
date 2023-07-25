/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	const struct device *dev;
	struct sensor_value co2, voc;

	dev = DEVICE_DT_GET_ONE(ams_iaqcore);
	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
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
	return 0;
}
