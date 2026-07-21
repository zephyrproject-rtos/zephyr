/*
 * Copyright (c) 2025 Thiyagarajan Pandiyan <psvthiyagarajan@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#define DISTANCE_ALIAS(i) DT_ALIAS(_CONCAT(distance, i))
#define DISTANCE_SENSOR(i, _)                                                                      \
	IF_ENABLED(DT_NODE_EXISTS(DISTANCE_ALIAS(i)), (DEVICE_DT_GET(DISTANCE_ALIAS(i)),))

/* support up to 5 distance sensors */
static const struct device *const sensors[] = {LISTIFY(5, DISTANCE_SENSOR, ())};

static void fetch_and_display(const struct device *sensor)
{
	struct sensor_value distance;
	int rc = sensor_sample_fetch(sensor);

	if (rc < 0) {
		printf("ERROR: Fetch failed: %d\n", rc);
		return;
	}

	rc = sensor_channel_get(sensor, SENSOR_CHAN_DISTANCE, &distance);
	if (rc < 0) {
		printf("ERROR: get failed: %d\n", rc);
	} else {
		printf("%s: %d.%03dm\n", sensor->name, distance.val1, distance.val2);
	}
}

int main(void)
{
	size_t i = 0;

	for (i = 0; i < ARRAY_SIZE(sensors); i++) {
		if (!device_is_ready(sensors[i])) {
			printf("Error: Device \"%s\" is not ready\n", sensors[i]->name);
			return 0;
		}
	}

	while (true) {
		for (i = 0; i < ARRAY_SIZE(sensors); i++) {
			fetch_and_display(sensors[i]);
			k_sleep(K_MSEC(500));
		}
	}

	return 0;
}
