/*
 * Copyright (c) 2019 Centaur Analytics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

void main(void)
{
	printk("Running on %s!\n", CONFIG_ARCH);
	const struct device *dev = device_get_binding(DT_LABEL(DT_INST(0, ti_hdc)));

	__ASSERT(dev != NULL, "Failed to get device binding");

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
}
