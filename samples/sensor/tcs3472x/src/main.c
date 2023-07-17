/*
 * Copyright (c) 2023, Thomas Kiss <thokis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

static const struct device *get_tcs3472x_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(ams_tcs3472x);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no TCS3472x found.\n");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: \"%s\" is not ready\n",
		       dev->name);
		return NULL;
	}

	printk("Found \"%s\", getting RGB values\n", dev->name);
	return dev;
}

int main(void)
{
	const struct device *dev = get_tcs3472x_device();

	if (dev == NULL) {
		return 0;
	}

	while (1) {

		struct sensor_value red, green, blue;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_RED, &red);
		sensor_channel_get(dev, SENSOR_CHAN_GREEN, &green);
		sensor_channel_get(dev, SENSOR_CHAN_BLUE, &blue);

		printk("red: %d.%06d; green: %d.%06d; blue: %d.%06d\n", red.val1, red.val2,
									green.val1, green.val2,
									blue.val1, blue.val2);

		k_sleep(K_MSEC(1000));
	}
	return 0;
}
