/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

void main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(infineon_thermistor);
	struct sensor_value value;
	int return_val;

	if (!device_is_ready(dev)) {
		printk("Thermistor is not ready\n");
		return;
	}

	printk("Thermistor example\n");

	while (true) {
		/* fetch sensor samples */
		return_val = sensor_sample_fetch(dev);
		if (return_val) {
			printk("Failed to fetch sample (%d)\n", return_val);
			continue;
		}

		return_val = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &value);
		if (return_val) {
			printk("Failed to get data (%d)\n", return_val);
			continue;
		}

		printk("Temperature: %.2f C\n\n", sensor_value_to_double(&value));

		k_sleep(K_MSEC(1000));
	}
}
