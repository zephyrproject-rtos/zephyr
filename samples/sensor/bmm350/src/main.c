/*
 * Copyright (c) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#if (CONFIG_BMM350_TRIGGER_GLOBAL_THREAD || CONFIG_BMM350_TRIGGER_OWN_THREAD)
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	struct sensor_value mag[3];
	float report_mag[3] = {0.0};
	int64_t current_time = k_uptime_get();

	sensor_sample_fetch(dev);

	sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, mag);

	report_mag[0] = sensor_value_to_double(mag[0]);
	report_mag[1] = sensor_value_to_double(mag[1]);
	report_mag[2] = sensor_value_to_double(mag[2]);
	printf("Data_ready TIME(ms) %lld MX: %f MY: %f MZ: %f\n",
	       current_time, report_mag[0], report_mag[1], report_mag[2]);
}
#endif

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(bosch_bmm350);
	struct sensor_value sampling_freq, oversampling;

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}


	printf("Device %p name is %s\n", dev, dev->name);

	/* Setting scale in G, due to loss of precision if the SI unit m/s^2
	 * is used
	 */
	sampling_freq.val1 = 25;       /* Hz from 0.78 to 400*/
	sampling_freq.val2 = 0;			 /*not use*/
	oversampling.val1 = 1;          /*1 Normal mode  0suspend mode*/
	oversampling.val2 = 0;          /*not use*/
	sensor_attr_set(dev, SENSOR_CHAN_MAGN_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change to 0.0Hz before changing
	 * other attributes
	 */
	sensor_attr_set(dev, SENSOR_CHAN_MAGN_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);


#if (CONFIG_BMM350_TRIGGER_GLOBAL_THREAD || CONFIG_BMM350_TRIGGER_OWN_THREAD)
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_MAGN_XYZ;

	if (sensor_trigger_set(dev, &trig, trigger_handler) != 0) {
		printf("Could not set sensor type and channel\n");
		return 0;
	}
#else
	int64_t current_time;
	double report_mag[3] = {0.0};

	while (1) {
		struct sensor_value mag[3];
		/* 10ms period, 100Hz Sampling frequency */
		k_sleep(K_MSEC(1000 / sampling_freq.val1));
		current_time = k_uptime_get();

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, mag);

		report_mag[0] = sensor_value_to_double(mag[0]);
		report_mag[1] = sensor_value_to_double(mag[1]);
		report_mag[2] = sensor_value_to_double(mag[2]);

		printf("Polling TIME(ms) %lld MX: %f MY: %f MZ: %f\n",
			current_time, report_mag[0], report_mag[1], report_mag[2]);
	}
#endif
	return 0;
}
