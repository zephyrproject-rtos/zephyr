/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#include <zephyr/drivers/sensor/sht4x.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

int main(void)
{
	const struct device *const sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
	struct sensor_value temp, hum;

	if (!device_is_ready(sht)) {
		printf("Device %s is not ready.\n", sht->name);
		return 0;
	}

#if CONFIG_APP_USE_HEATER
	struct sensor_value heater_p;
	struct sensor_value heater_d;

	heater_p.val1 = CONFIG_APP_HEATER_PULSE_POWER;
	heater_d.val1 = !!CONFIG_APP_HEATER_PULSE_DURATION_LONG;
	sensor_attr_set(sht, SENSOR_CHAN_ALL, SENSOR_ATTR_SHT4X_HEATER_POWER, &heater_p);
	sensor_attr_set(sht, SENSOR_CHAN_ALL, SENSOR_ATTR_SHT4X_HEATER_DURATION, &heater_d);
#endif

	while (true) {
		if (sensor_sample_fetch(sht)) {
			printf("Failed to fetch sample from SHT4X device\n");
			return 0;
		}

		sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);

#if CONFIG_APP_USE_HEATER
		/*
		 * Conditions in which it makes sense to activate the heater
		 * are application/environment specific.
		 *
		 * The heater should not be used above SHT4X_HEATER_MAX_TEMP (65 °C)
		 * as stated in the datasheet.
		 *
		 * The temperature data will not be updated here for obvious reasons.
		 **/
		if (hum.val1 > CONFIG_APP_HEATER_HUMIDITY_THRESH &&
		    temp.val1 < SHT4X_HEATER_MAX_TEMP) {
			printf("Activating heater.\n");

			if (sht4x_fetch_with_heater(sht)) {
				printf("Failed to fetch sample from SHT4X device\n");
				return 0;
			}

			sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);
		}
#endif

		printf("SHT4X: %.2f Temp. [C] ; %0.2f RH [%%]\n", sensor_value_to_double(&temp),
		       sensor_value_to_double(&hum));

#if CONFIG_APP_USE_HEATER && !CONFIG_APP_HEATER_PULSE_DURATION
		k_sleep(K_MSEC(20000));
#else
		k_sleep(K_MSEC(2000));
#endif
	}
}
