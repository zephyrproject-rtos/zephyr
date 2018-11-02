/*
 * Copyright (c) 2018 Laird
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <stdio.h>

void main(void)
{
	struct device *dev = device_get_binding("BME680");
	int err;
	struct sensor_value temperature, pressure, humidity;
#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
	struct sensor_value gas;
#endif

	printf("dev %p name %s\n", dev, dev->config->name);
#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
	printf(" -> Gas sensor enabled\n");
#endif

	while (1) {
		err = sensor_sample_fetch(dev);
		if (err == 0) {
			sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
			sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);
			sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
			sensor_channel_get(dev, SENSOR_CHAN_GAS, &gas);
#endif

			printf("temperature: %d.%02d\u00B0C; pressure: %d.%03dKPa; "
			      "humidity: %d.%03d%%"
#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
			      "; gas: %d\u2126"
#endif
			      "\n",
			      temperature.val1, temperature.val2, pressure.val1,
			      pressure.val2, humidity.val1, humidity.val2
#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
			      , gas.val1
#endif
			      );
		}
		else
		{
			printf("Failed to read sensor, error code: %d\n", err);
		}

		k_sleep(8000);
	}
}

