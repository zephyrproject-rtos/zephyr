/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <usb/usb_device.h>

static void fetch_and_display(const struct device *sensor)
{
	static unsigned int count;
	struct sensor_value accel[3];
	struct sensor_value temperature;
	const char *overrun = "";
	int rc = sensor_sample_fetch(sensor);

	++count;
	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_BMA400_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor,
					SENSOR_CHAN_ACCEL_XYZ,
					accel);
	}
	if (rc < 0) {
		printf("ERROR: Update failed: %d\n", rc);
	} else {
		printf("#%u @ %u ms: %sx %f , y %f , z %f",
		       count, k_uptime_get_32(), overrun,
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));
	}

    if (rc == 0) {
        rc = sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &temperature);
        if (rc < 0) {
            printf("\nERROR: Unable to read temperature:%d\n", rc);
        } else {
            printf(", t %f\n", sensor_value_to_double(&temperature));
        }
    }
}

#ifdef CONFIG_BMA400_TRIGGER
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	fetch_and_display(dev);
}
#endif

void main(void)
{
	/* Init USB for UART console */
	usb_enable(NULL);

	const struct device *sensor = DEVICE_DT_GET_ANY(bosch_bma400);

	if (sensor == NULL) {
		printf("No device found\n");
		return;
	}
	if (!device_is_ready(sensor)) {
		printf("Device %s is not ready\n", sensor->name);
		return;
	}

#if CONFIG_BMA400_TRIGGER
	{
		struct sensor_trigger trig;
		int rc;

		enum sensor_attribute attr = SENSOR_ATTR_LOWER_THRESH;
		const struct sensor_value val = { .val1 = 0, .val2 = 0 }; // TODO: Give these proper values

		rc = sensor_attr_set(sensor, SENSOR_CHAN_ACCEL_XYZ, attr, &val);
		if (rc != 0) {
			printf("Failed to set sensor attribute: %d\n", rc);
			return;
		}


		trig.type = SENSOR_TRIG_DATA_READY;
		// trig.type = SENSOR_TRIG_THRESHOLD;
		trig.chan = SENSOR_CHAN_ACCEL_XYZ;

		rc = sensor_trigger_set(sensor, &trig, trigger_handler);
		if (rc != 0) {
			printf("Failed to set trigger: %d\n", rc);
			return;
		}

		printf("Waiting for triggers\n");
		while (true) {
			k_sleep(K_MSEC(2000));
		}
	}
#else /* CONFIG_BMA400_TRIGGER */
	printf("Polling at 0.5 Hz\n");
	while (true) {
		fetch_and_display(sensor);
		k_sleep(K_MSEC(2000));
	}
#endif /* CONFIG_BMA400_TRIGGER */
}
