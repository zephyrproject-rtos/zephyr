/*
 * Copyright (c) 2019 Centaur Analytics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

int main(void)
{
	printk("Running on %s!\n", CONFIG_ARCH);
	const struct device *const dev = DEVICE_DT_GET_ONE(ti_hdc);

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	printk("Dev %p name %s is ready!\n", dev, dev->name);

	struct sensor_value temp, humidity;

	while (1) {
		/* take a sample */
		printk("Fetching...\n");
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);

		/* print the result */
		printk("Temp = %d.%06d C, RH = %d.%06d %%\n",
		       temp.val1, temp.val2, humidity.val1, humidity.val2);

		/* wait for the next sample */
		k_sleep(K_SECONDS(10));
	}
	return 0;
}
