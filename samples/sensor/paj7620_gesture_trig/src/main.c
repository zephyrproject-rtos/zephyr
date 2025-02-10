/*
 * Copyright (c) Paul Timke <ptimkec@live.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/paj7620.h>

K_SEM_DEFINE(sem, 0, 1); /* starts off "not available" */

static void trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	ARG_UNUSED(trigger);

	if (sensor_sample_fetch(dev) < 0) {
		printf("sensor_sample_fetch failed\n");
		return;
	}

	k_sem_give(&sem);
}

int main(void)
{
	int ret;
	struct sensor_value data;
	const struct device *dev = DEVICE_DT_GET_ONE(pixart_paj7620);

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_MOTION,
		.chan = (enum sensor_channel)SENSOR_CHAN_PAJ7620_GESTURES,
	};

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return -ENODEV;
	}

	ret = sensor_trigger_set(dev, &trig, trigger_handler);
	if (ret < 0) {
		printf("Could not set trigger\n");
		return ret;
	}

	while (1) {
		k_sem_take(&sem, K_FOREVER);
		sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_PAJ7620_GESTURES, &data);

		switch (data.val1) {
		case PAJ7620_GES_UP:
			printf("Gesture UP\n");
			break;
		case PAJ7620_GES_DOWN:
			printf("Gesture DOWN\n");
			break;
		case PAJ7620_GES_LEFT:
			printf("Gesture LEFT\n");
			break;
		case PAJ7620_GES_RIGHT:
			printf("Gesture RIGHT\n");
			break;
		case PAJ7620_GES_FORWARD:
			printf("Gesture FORWARD\n");
			break;
		case PAJ7620_GES_BACKWARD:
			printf("Gesture BACKWARD\n");
			break;
		case PAJ7620_GES_CLOCKWISE:
			printf("Gesture CLOCKWISE\n");
			break;
		case PAJ7620_GES_COUNTERCLOCKWISE:
			printf("Gesture COUNTER CLOCKWISE\n");
			break;
		default:
			break;
		}
	}
}
