/*
 * Copyright (c) 2019 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

void main(void)
{
	printk("Hello DPS310\n");
	const struct device *dev = DEVICE_DT_GET_ONE(infineon_dps310);

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return;
	}

	printk("dev %p name %s\n", dev, dev->name);

	while (1) {
		struct sensor_value temp, press;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);

		printk("temp: %d.%06d; press: %d.%06d\n",
		      temp.val1, abs(temp.val2), press.val1, press.val2);

		k_sleep(K_MSEC(1000));
	}
}
