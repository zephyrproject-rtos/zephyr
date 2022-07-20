/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

void main(void)
{
	struct sensor_value val;
	int rc;
	const struct device *dev = DEVICE_DT_GET_ONE(st_stm32_vbat);

	if (!device_is_ready(dev)) {
		printk("VBAT sensor is not ready\n");
		return;
	}

	printk("STM32 Vbat sensor test\n");

	/* fetch sensor samples */
	rc = sensor_sample_fetch(dev);
	if (rc) {
		printk("Failed to fetch sample (%d)\n", rc);
		return;
	}

	rc = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &val);
	if (rc) {
		printk("Failed to get data (%d)\n", rc);
		return;
	}

	printk("Current Vbat voltage: %.2f V\n", sensor_value_to_double(&val));
}
