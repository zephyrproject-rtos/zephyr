/*
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>

#define ALERT_HUMIDITY 40

#ifdef CONFIG_SHT3XD_TRIGGER
static volatile bool alerted;

static void trigger_handler(struct device *dev, struct sensor_trigger *trig)
{
	alerted = !alerted;
}

#endif

void main(void)
{
	struct device *dev = device_get_binding("SHT3XD");
	int rc;

	if (dev == NULL) {
		printf("Could not get SHT3XD device\n");
		return;
	}

#ifdef CONFIG_SHT3XD_TRIGGER
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_HUMIDITY,
	};
	struct sensor_value lo_thr = { 0 };
	struct sensor_value hi_thr = { ALERT_HUMIDITY };
	bool last_alerted = false;

	rc = sensor_attr_set(dev, SENSOR_CHAN_HUMIDITY,
			     SENSOR_ATTR_LOWER_THRESH, &lo_thr);
	if (rc == 0) {
		rc = sensor_attr_set(dev, SENSOR_CHAN_HUMIDITY,
				     SENSOR_ATTR_UPPER_THRESH, &hi_thr);
	}
	if (rc == 0) {
		rc = sensor_trigger_set(dev, &trig, trigger_handler);
	}
	printf("Alert outside %d..%d %%RH got %d\n", lo_thr.val1,
	       hi_thr.val1, rc);
#endif

	while (true) {
		struct sensor_value temp, hum;

#ifdef CONFIG_SHT3XD_TRIGGER
		if (alerted != last_alerted) {
			static const char *const alert_str[] = {
				"below",
				"above",
			};
			printf("Humidity %s %d!\n", alert_str[alerted],
			       hi_thr.val1);
			last_alerted = alerted;
		}
#endif
		rc = sensor_sample_fetch(dev);
		if (rc == 0) {
			rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
						&temp);
		}
		if (rc == 0) {
			rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY,
						&hum);
		}
		if (rc != 0) {
			printf("SHT3XD: failed: %d\n", rc);
			break;
		}
		printf("SHT3XD: %.2f Cel ; %0.2f %%RH\n",
		       sensor_value_to_double(&temp),
		       sensor_value_to_double(&hum));

		k_sleep(K_MSEC(2000));
	}
}
