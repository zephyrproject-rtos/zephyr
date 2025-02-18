/*
 * Copyright (c) Paul Timke <ptimkec@live.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/paj7620.h>

#define GESTURE_POLL_TIME_MS 100

static void print_hand_gesture(const struct device *dev)
{
	int ret;
	struct sensor_value data;

	ret = sensor_sample_fetch(dev);
	if (ret < 0) {
		printf("Failed to fetch from device");
	}

	ret = sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_PAJ7620_GESTURES, &data);
	if (ret < 0) {
		printf("Failed to get channel from device");
		return;
	}

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
		printf("Gesture COUNTERCLOCKWISE\n");
		break;
	default:
		break;
	}
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(pixart_paj7620);

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return -ENODEV;
	}

	while (1) {
		print_hand_gesture(dev);
		k_msleep(GESTURE_POLL_TIME_MS);
	}

	return 0;
}
