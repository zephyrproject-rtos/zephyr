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

#ifdef CONFIG_SAMPLE_TAP_DETECTION
static void tap_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	ARG_UNUSED(trigger);
	printf("TAP detected\n");

	if (sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ) < 0) {
		printf("ERROR: SENSOR_CHAN_ACCEL_XYZ fetch failed\n");
	}

	k_sem_give(&sem);
}
#else
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
#endif

int main(void)
{
	struct sensor_value data[3];
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(accel0));

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}

#ifdef CONFIG_SAMPLE_TAP_DETECTION
	trig.type = SENSOR_TRIG_DOUBLE_TAP;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	if (sensor_trigger_set(dev, &trig, tap_trigger_handler) < 0) {
		printf("Could not set tap trigger\n");
		return 0;
	}
#else
	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger\n");
		return 0;
	}
#endif

	while (1) {
		k_sem_take(&sem, K_FOREVER);
		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, data);

		/* Print accel x,y,z data */
		printf("%16s [m/s^2]:    (%12.6f, %12.6f, %12.6f)\n", dev->name,
		       sensor_value_to_double(&data[0]), sensor_value_to_double(&data[1]),
		       sensor_value_to_double(&data[2]));
	}
}
