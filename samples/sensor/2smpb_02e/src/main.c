/*
 * Copyright (c) 2025 CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(omron_2smpb_02e);
	struct sensor_value value;

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return 1;
	}

	while (1) {
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &value);
		printk("Temperature: %d.%d°C\n", value.val1, value.val2);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &value);
		printk("Pressure: %d.%dhPa\n", value.val1, value.val2);

		k_sleep(K_MSEC(1000));
	}
}
