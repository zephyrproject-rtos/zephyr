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

	int rc = sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);

	if (rc < 0) {
		printf("ERROR: SENSOR_CHAN_ACCEL_XYZ fetch failed: %d\n", rc);
	}

	k_sem_give(&sem);
}
#elif defined(CONFIG_ACTIVITY_DETECTION)
static void activity_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	ARG_UNUSED(trigger);
	printf("ACTIVITY detected\n");

	int rc = sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);

	if (rc < 0) {
		printf("ERROR: SENSOR_CHAN_ACCEL_XYZ fetch failed: %d\n", rc);
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
	int rc = sensor_sample_fetch(dev);

	if (rc) {
		printf("sensor_sample_fetch failed: %d\n", rc);
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
	int rc = sensor_trigger_set(dev, &trig, tap_trigger_handler);

	if (rc < 0) {
		printf("Could not set tap trigger: %d\n", rc);
		return 0;
	}
#elif defined(CONFIG_ACTIVITY_DETECTION)
	struct sensor_value thresh = {
		.val1 = 15,
		.val2 = 0
	};

	int rc = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
				 SENSOR_ATTR_UPPER_THRESH, &thresh);
	if (rc < 0) {
		printf("Failed to set activity threshold: %d\n", rc);
		return 0;
	}

	trig.type = SENSOR_TRIG_MOTION;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	rc = sensor_trigger_set(dev, &trig, activity_trigger_handler);

	if (rc < 0) {
		printf("Could not set activity trigger: %d\n", rc);
		return 0;
	}
#else
	int rc = sensor_trigger_set(dev, &trig, trigger_handler);

	if (rc) {
		printf("Could not set trigger: %d\n", rc);
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
