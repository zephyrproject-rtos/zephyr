/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PRESSURE_POLLING, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Get a device structure from a devicetree node from alias
 * "pressure_sensor".
 */
static const struct device *get_pressure_sensor_device(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;
}

int main(void)
{
	const struct device *dev = get_pressure_sensor_device();

	if (dev == NULL) {
		return 0;
	}
	struct sensor_value pressure;
	struct sensor_value temperature;
	struct sensor_value altitude;

	LOG_INF("Starting pressure and altitude polling sample.\n");

	while (1) {
		if (sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL) == 0) {
			sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);
			sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
			sensor_channel_get(dev, SENSOR_CHAN_ALTITUDE, &altitude);

			LOG_INF("temp %.2f Cel, pressure %f kPa, altitude %f m",
				sensor_value_to_double(&temperature),
				sensor_value_to_double(&pressure),
				sensor_value_to_double(&altitude));
		}
	}
	return 0;
}
