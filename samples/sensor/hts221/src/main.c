/*
 * Copyright (c) 2017 Linaro Limited
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
	struct sensor_value temp, hum;
	if (sensor_sample_fetch(dev) < 0) {
		printf("Sensor sample update error\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
		printf("Cannot read HTS221 temperature channel\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
		printf("Cannot read HTS221 humidity channel\n");
		return;
	}

	++obs;
	printf("Observation:%u\n", obs);

	/* display temperature */
	printf("Temperature:%.1f C\n", sensor_value_to_double(&temp));

	/* display humidity */
	printf("Relative Humidity:%.1f%%\n",
	       sensor_value_to_double(&hum));
}

static void hts221_handler(const struct device *dev,
			   const struct sensor_trigger *trig)
{
	process_sample(dev);
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(st_hts221);

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	if (IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ALL,
		};
		if (sensor_trigger_set(dev, &trig, hts221_handler) < 0) {
			printf("Cannot configure trigger\n");
			return 0;
		}
	}

	while (!IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
		process_sample(dev);
		k_sleep(K_MSEC(2000));
	}
	k_sleep(K_FOREVER);
	return 0;
}
