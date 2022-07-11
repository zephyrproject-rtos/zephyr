/*
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#define ALERT_HUMIDITY_LO 50
#define ALERT_HUMIDITY_HI 60

#ifdef CONFIG_SHT3XD_TRIGGER
static volatile bool alerted;

static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	alerted = !alerted;
}

#endif

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(sensirion_sht3xd);
	int rc;

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return;
	}

#ifdef CONFIG_SHT3XD_TRIGGER
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_HUMIDITY,
	};
	struct sensor_value lo_thr = { ALERT_HUMIDITY_LO };
	struct sensor_value hi_thr = { ALERT_HUMIDITY_HI };
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
	if (rc != 0) {
		printf("SHT3XD: trigger config failed: %d\n", rc);
		return;
	}
	printf("Alert outside %d..%d %%RH got %d\n", lo_thr.val1,
	       hi_thr.val1, rc);
#endif

	while (true) {
		struct sensor_value temp, hum;

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

#ifdef CONFIG_SHT3XD_TRIGGER
		if (alerted != last_alerted) {
			if (lo_thr.val1 > hum.val1) {
				printf("ALERT: humidity %d < %d\n",
				       hum.val1, lo_thr.val1);
			} else if (hi_thr.val1 < hum.val1) {
				printf("ALERT: humidity %d > %d\n",
				       hum.val1, hi_thr.val1);
			} else {
				printf("ALERT: humidity %d <= %d <= %d\n",
				       lo_thr.val1, hum.val1, hi_thr.val1);
			}
			last_alerted = alerted;
		}
#endif

		printf("SHT3XD: %.2f Cel ; %0.2f %%RH\n",
		       sensor_value_to_double(&temp),
		       sensor_value_to_double(&hum));

		k_sleep(K_MSEC(2000));
	}
}
