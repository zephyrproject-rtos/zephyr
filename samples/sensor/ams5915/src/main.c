/*
 * Copyright (c) 2024 BoRob Engineering
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(analogmicro_ams5915);
	struct sensor_value temp, press;

	__ASSERT(dev != NULL, "Failed to get device binding");
	__ASSERT(device_is_ready(dev), "Device %s is not ready", dev->name);

	printk("device is %p, name is %s\n", dev, dev->name);

	while (1) {
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);

		printk("temp: %.6f; press: %.6f;\n", sensor_value_to_double(&temp),
		       sensor_value_to_double(&press));

		k_sleep(K_MSEC(100));
	}

	return 0;
}
