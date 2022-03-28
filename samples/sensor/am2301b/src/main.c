/*
 * Copyright (c) 2022 Hisanori YAMAOKU
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

void main(void)
{
	/*
	 * Get a device structure from a devicetree node with compatible
	 * "aosong,am2301b". (If there are multiple, just pick one.)
	 */
	const struct device *dev = DEVICE_DT_GET_ANY(aosong_am2301b);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);

	while (1) {
		struct sensor_value temp, humidity;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);

		printk("temp: %d.%06d; humidity: %d.%06d\n",
		       temp.val1, temp.val2, humidity.val1, humidity.val2);

		k_sleep(K_MSEC(1000));
	}
}
