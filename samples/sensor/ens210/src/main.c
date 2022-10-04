/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(ams_ens210);
	struct sensor_value temperature, humidity;

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->name);

	while (1) {
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
		printk("Temperature: %d.%06d C; Humidity: %d.%06d%%\n",
			temperature.val1, temperature.val2,
			humidity.val1, humidity.val2);

		k_sleep(K_MSEC(1000));
	}
}
