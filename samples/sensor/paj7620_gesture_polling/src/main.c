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
		return;
	}

	ret = sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_PAJ7620_GESTURES, &data);
	if (ret < 0) {
		printf("Failed to get channel from device");
		return;
	}

	if ((data.val1 & PAJ7620_FLAG_GES_UP) != 0) {
		printf("Gesture UP\n");
	}
	if ((data.val1 & PAJ7620_FLAG_GES_DOWN) != 0) {
		printf("Gesture DOWN\n");
	}
	if ((data.val1 & PAJ7620_FLAG_GES_LEFT) != 0) {
		printf("Gesture LEFT\n");
	}
	if ((data.val1 & PAJ7620_FLAG_GES_RIGHT) != 0) {
		printf("Gesture RIGHT\n");
	}
	if ((data.val1 & PAJ7620_FLAG_GES_FORWARD) != 0) {
		printf("Gesture FORWARD\n");
	}
	if ((data.val1 & PAJ7620_FLAG_GES_BACKWARD) != 0) {
		printf("Gesture BACKWARD\n");
	}
	if ((data.val1 & PAJ7620_FLAG_GES_CLOCKWISE) != 0) {
		printf("Gesture CLOCKWISE\n");
	}
	if ((data.val1 & PAJ7620_FLAG_GES_COUNTERCLOCKWISE) != 0) {
		printf("Gesture COUNTER CLOCKWISE\n");
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
