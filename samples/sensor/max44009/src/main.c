/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>

/**
 * @file Sample app using the MAX44009 light sensor through ARC I2C.
 *
 * This app will read the sensor reading via I2C interface and show
 * the result every 4 seconds.
 */

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(maxim_max44009);
	struct sensor_value val;
	uint32_t lum = 0U;

	printk("MAX44009 light sensor application\n");

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return 0;
	}

	while (1) {
		if (sensor_sample_fetch_chan(dev, SENSOR_CHAN_LIGHT) != 0) {
			printk("sensor: sample fetch fail.\n");
			return 0;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &val) != 0) {
			printk("sensor: channel get fail.\n");
			return 0;
		}

		lum = val.val1;
		printk("sensor: lum reading: %d\n", lum);

		k_sleep(K_MSEC(4000));
	}
	return 0;
}
