/*
 * Copyright (c) 2026 William Fish (Manulytica ltd)
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

int main(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(adi_ad74115h);

	if (!device_is_ready(dev)) {
		printk("Device not ready\n");
		return 0;
	}

	printk("AD74115H Sample Started\n");

	while (1) {
		struct sensor_value val;

		if (sensor_sample_fetch(dev) == 0) {
			sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &val);
			printk("ADC Voltage: %d.%06d V\n", val.val1, val.val2);
		}

		k_sleep(K_MSEC(1000));
	}
	return 0;
}
