/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <usb/usb_device.h>

#define MEASURE_WITH_COMPENSATION 1

void main(void)
{
#if defined(CONFIG_USB_UART_CONSOLE)
	const struct device *dev;

	dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	if (dev == NULL || usb_enable(NULL)) {
		return;
	}
#endif
	const struct device *sht = device_get_binding("SHT4X");
	const struct device *sgp = device_get_binding("SGP40");

#if MEASURE_WITH_COMPENSATION
	struct sensor_value comp_data = {
		.val1 = 50, // RH [%]
		.val2 = 25  // Temp [°C]
	};
#endif

	if (sht == NULL) {
		printf("Could not get SHT4X device\n");
		return;
	}

	if (sgp == NULL) {
		printf("Could not get SGP40 device\n");
		return;
	}

	while (true) {
		struct sensor_value temp, hum, gas;

		if ( sensor_sample_fetch(sht) ) {
			printf("Failed to fetch sample from SHT4X device\n");
			return;
		} else {
			sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);
		}

		if ( sensor_sample_fetch(sgp) ) {
			printf("Failed to fetch sample from SGP40 device\n");
			return;
		} else {
#if MEASURE_WITH_COMPENSATION
			comp_data.val1 = hum.val1;
			comp_data.val2 = temp.val1;
			sensor_attr_set(sgp, SENSOR_CHAN_GAS_RES,
					SENSOR_ATTR_OFFSET, &comp_data);
#endif
			sensor_channel_get(sgp, SENSOR_CHAN_GAS_RES, &gas);
		}

		printf("SHT4X: %.2f Temp. [C] ; %0.2f RH [%%] -- SGP40: %.0f Gas [a.u.]\n",
		       sensor_value_to_double(&temp),
		       sensor_value_to_double(&hum),
		       sensor_value_to_double(&gas));

		k_sleep(K_MSEC(2000));
	}
	return;
}
