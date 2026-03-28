/*
 * Copyright (c) 2017, NXP
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#define MAX30101_SENSOR_CHANNEL SENSOR_CHAN_GREEN

static void print_sample_fetch(const struct device *dev)
{
	static struct sensor_value green;

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, MAX30101_SENSOR_CHANNEL, &green);

	/* Print LED data*/
	printf("GREEN = %d\n", green.val1);
}

#if CONFIG_MAX30101_TRIGGER
static struct sensor_trigger trig_drdy;

void sensor_data_ready(const struct device *dev, const struct sensor_trigger *trigger)
{
	print_sample_fetch(dev);
}
#endif /* CONFIG_MAX30101_TRIGGER */

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(heart_rate_sensor));

	if (dev == NULL) {
		printf("Could not get heart_rate_sensor\n");
		return 0;
	}
	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}

#if CONFIG_MAX30101_TRIGGER
	trig_drdy.type = SENSOR_TRIG_DATA_READY;
	trig_drdy.chan = MAX30101_SENSOR_CHANNEL;
	sensor_trigger_set(dev, &trig_drdy, sensor_data_ready);
#endif /* CONFIG_MAX30101_TRIGGER */

	while (1) {
#if !CONFIG_MAX30101_TRIGGER
		print_sample_fetch(dev);
#endif /* !CONFIG_MAX30101_TRIGGER */

		k_sleep(K_MSEC(20));
	}
	return 0;
}
