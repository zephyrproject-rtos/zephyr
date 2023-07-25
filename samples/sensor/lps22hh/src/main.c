/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

static void process_sample(const struct device *dev)
{
	static unsigned int obs;
	struct sensor_value pressure, temp;

	if (sensor_sample_fetch(dev) < 0) {
		printf("Sensor sample update error\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure) < 0) {
		printf("Cannot read LPS22HH pressure channel\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
		printf("Cannot read LPS22HH temperature channel\n");
		return;
	}

	++obs;
	printf("Observation: %u\n", obs);

	/* display pressure */
	printf("Pressure: %.3f kPa\n", sensor_value_to_double(&pressure));

	/* display temperature */
	printf("Temperature: %.2f C\n", sensor_value_to_double(&temp));

}

static void lps22hh_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	process_sample(dev);
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lps22hh);

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	if (IS_ENABLED(CONFIG_LPS22HH_TRIGGER)) {
		struct sensor_value attr = {
			.val1 = 1,
		};
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ALL,
		};

		if (sensor_attr_set(dev, SENSOR_CHAN_ALL,
				    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr) < 0) {
			printf("Cannot configure sampling rate\n");
			return 0;
		}
		if (sensor_trigger_set(dev, &trig, lps22hh_handler) < 0) {
			printf("Cannot configure trigger\n");
			return 0;
		}
		printk("Configured for triggered collection at %u Hz\n",
		       attr.val1);
	}

	while (!IS_ENABLED(CONFIG_LPS22HH_TRIGGER)) {
		process_sample(dev);
		k_sleep(K_MSEC(2000));
	}
	k_sleep(K_FOREVER);
	return 0;
}
