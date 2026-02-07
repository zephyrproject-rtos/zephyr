/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(bosch_bhi2xy);
	struct sensor_value game_rv[4];
	struct sensor_value sampling_freq;

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}

	printf("Device %p name is %s\n", dev, dev->name);

	sensor_value_from_float(&sampling_freq, 50.0f); /* 50Hz */
	sensor_attr_set(dev, SENSOR_CHAN_GAME_ROTATION_VECTOR, SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);

	while (1) {
		/* 20ms period, 50Hz Sampling frequency */
		k_sleep(K_MSEC(20));

		sensor_sample_fetch(dev);

		sensor_channel_get(dev, SENSOR_CHAN_GAME_ROTATION_VECTOR_X, &game_rv[0]);
		sensor_channel_get(dev, SENSOR_CHAN_GAME_ROTATION_VECTOR_Y, &game_rv[1]);
		sensor_channel_get(dev, SENSOR_CHAN_GAME_ROTATION_VECTOR_Z, &game_rv[2]);
		sensor_channel_get(dev, SENSOR_CHAN_GAME_ROTATION_VECTOR_W, &game_rv[3]);

		printf("GRVX: %c%d.%06d; GRVY: %c%d.%06d; GRVZ: %c%d.%06d; GRVW: %c%d.%06d\n",
		       (game_rv[0].val1 < 0 || game_rv[0].val2 < 0) ? '-' : ' ',
		       ABS(game_rv[0].val1), ABS(game_rv[0].val2),
		       (game_rv[1].val1 < 0 || game_rv[1].val2 < 0) ? '-' : ' ',
		       ABS(game_rv[1].val1), ABS(game_rv[1].val2),
		       (game_rv[2].val1 < 0 || game_rv[2].val2 < 0) ? '-' : ' ',
		       ABS(game_rv[2].val1), ABS(game_rv[2].val2),
		       (game_rv[3].val1 < 0 || game_rv[3].val2 < 0) ? '-' : ' ',
		       ABS(game_rv[3].val1), ABS(game_rv[3].val2));
	}
	return 0;
}
