/*
 * Copyright 2024 NXP
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

K_SEM_DEFINE(sem, 0, 1); /* starts off "not available" */

static void trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	ARG_UNUSED(trigger);

	/* Always fetch the sample to clear the data ready interrupt in the
	 * sensor.
	 */
	if (sensor_sample_fetch(dev)) {
		printf("sensor_sample_fetch failed\n");
		return;
	}

	k_sem_give(&sem);
}

int main(void)
{
	struct sensor_value data[3];
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(magn0));

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_MAGN_XYZ,
	};

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger\n");
		return 0;
	}

	while (1) {
		k_sem_take(&sem, K_FOREVER);
		sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, data);

		/* Print magn x,y,z data */
		printf("%16s (x, y, z):    (%12.6f, %12.6f, %12.6f)\n", dev->name,
		       sensor_value_to_double(&data[0]), sensor_value_to_double(&data[1]),
		       sensor_value_to_double(&data[2]));
	}
}
