/*
 * Copyright (c) 2020 Ioannis Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <math.h>

struct sample_data {
	struct device *bmp388_dev;
	float zero_pressure;
};

struct sample_data data;

int zero_pressure(void)
{
	int i;
	struct sensor_value press;

	/* Dropout the first 10 measurements to initialize the filter */
	for (i = 0; i < 10; i++) {
		sensor_sample_fetch(data.bmp388_dev);
		k_sleep(K_MSEC(100));
	}

	data.zero_pressure = 0.f;
	for (i = 0; i < 10; i++) {
		sensor_sample_fetch(data.bmp388_dev);
		sensor_channel_get(data.bmp388_dev, SENSOR_CHAN_PRESS, &press);

		data.zero_pressure += sensor_value_to_double(&press);
		k_sleep(K_MSEC(100));
	}

	data.zero_pressure /= 10;

	return 0;
}

float calculate_altitude(float pressure, float zero_p, float temperature)
{
	return (1.f - pow(pressure / zero_p, 0.19022256f)) *
	       (temperature + 273.15f) / 0.0065f;
}

static void bmp388_config(struct device *bmp388)
{
	struct sensor_value osr_attr;

	/* set BMP388 pressure oversampling to x32 &
	 * temperature oversampling to x2
	 */
	osr_attr.val1 = 32;
	osr_attr.val2 = 2;

	if (sensor_attr_set(bmp388, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_OVERSAMPLING, &osr_attr) < 0) {
		printk("Cannot set oversampling options for BMP388\n");
		return;
	}
}

void main(void)
{
	data.bmp388_dev = device_get_binding("BMP388");
	if (!data.bmp388_dev) {
		printf("Could not get BMP388 device\n");
		return;
	}

	printf("dev %p name %s\n", data.bmp388_dev,
	       data.bmp388_dev->config->name);

	bmp388_config(data.bmp388_dev);

	printk("Zeroing the pressure\n");
	zero_pressure();

	while (1) {
		struct sensor_value temp, press;

		sensor_sample_fetch(data.bmp388_dev);
		sensor_channel_get(data.bmp388_dev, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(data.bmp388_dev, SENSOR_CHAN_AMBIENT_TEMP,
				   &temp);

		float pressure = sensor_value_to_double(&press);
		float temperature = sensor_value_to_double(&temp);

		printf("press: % 8.04f kPa; temp: % 6.02f oC",
		       pressure, temperature);
		printf("; altitude: %4.1f m\n",
		       calculate_altitude(pressure, data.zero_pressure,
					  temperature));

		k_sleep(K_MSEC(100));
	}
}
