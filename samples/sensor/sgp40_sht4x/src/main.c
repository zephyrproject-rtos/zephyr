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

/*
 * User configurable options
 */
#define MEASURE_WITH_COMPENSATION 0
#define USE_HEATER 1

#if MEASURE_WITH_COMPENSATION
/* public API for handling T/RH compensated measurements */
#include <drivers/sensor/sgp40.h>
#endif

#if USE_HEATER
/* public API handling the heater of SHT4X devices */
#include <drivers/sensor/sht4x.h>
/* Threshold above which the heater will be activated */
#define HEATER_HUM_THRESH 65
#endif

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sgp40)
#error "No sensirion,sgp40 compatible node found in the device tree"
#endif

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

void main(void)
{

#if MEASURE_WITH_COMPENSATION
	struct sensor_value comp_t;
	struct sensor_value comp_rh;
#endif
	const struct device *sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
	const struct device *sgp = DEVICE_DT_GET_ANY(sensirion_sgp40);
	struct sensor_value temp, hum, gas;

	if (!device_is_ready(sht)) {
		printf("Device %s is not ready.\n", sht->name);
		return;
	}

	if (!device_is_ready(sgp)) {
		printf("Device %s is not ready.\n", sgp->name);
		return;
	}

#if USE_HEATER
	struct sensor_value heater_p;
	struct sensor_value heater_d;

	heater_p.val1 = 0;
	heater_d.val1 = 0;
	sensor_attr_set(sht, SENSOR_CHAN_ALL, SENSOR_ATTR_SHT4X_HEATER_POWER, &heater_p);
	sensor_attr_set(sht, SENSOR_CHAN_ALL, SENSOR_ATTR_SHT4X_HEATER_DURATION, &heater_d);
#endif

	while (true) {

		if (sensor_sample_fetch(sht)) {
			printf("Failed to fetch sample from SHT4X device\n");
			return;
		}

		sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);
#if USE_HEATER
		/*
		 * Conditions in which it makes sense to activate the heater
		 * are application/environment specific.
		 *
		 * The heater should not be used above SHT4X_HEATER_MAX_TEMP (65 °C)
		 * as stated in the datasheet.
		 *
		 * The temperature data will not be updated here for obvious reasons.
		 */
		if (hum.val1 > HEATER_HUM_THRESH && temp.val1 < SHT4X_HEATER_MAX_TEMP) {
			printf("Activating heater.\n");

			if (sht4x_fetch_with_heater(sht)) {
				printf("Failed to fetch sample from SHT4X device\n");
				return;
			}

			sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);
		}
#endif

#if MEASURE_WITH_COMPENSATION
		comp_t.val1 = temp.val1; /* Temp [°C] */
		comp_rh.val1 = hum.val1; /* RH [%] */
		sensor_attr_set(sgp,
				SENSOR_CHAN_GAS_RES,
				SENSOR_ATTR_SGP40_TEMPERATURE,
				&comp_t);
		sensor_attr_set(sgp,
				SENSOR_CHAN_GAS_RES,
				SENSOR_ATTR_SGP40_HUMIDITY,
				&comp_rh);
#endif
		if (sensor_sample_fetch(sgp)) {
			printf("Failed to fetch sample from SGP40 device.\n");
			return;
		}

		sensor_channel_get(sgp, SENSOR_CHAN_GAS_RES, &gas);

		printf("SHT4X: %.2f Temp. [C] ; %0.2f RH [%%] -- SGP40: %d Gas [a.u.]\n",
		       sensor_value_to_double(&temp),
		       sensor_value_to_double(&hum),
		       gas.val1);

#if USE_HEATER /* maximum duty cycle for using the heater is 5% */
		k_sleep(K_MSEC(20000));
#else
		k_sleep(K_MSEC(2000));
#endif
	}
}
