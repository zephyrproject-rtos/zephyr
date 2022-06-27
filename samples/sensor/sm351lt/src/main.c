/*
 * Copyright (c) 2020 Laird Connectivity
 * Based on code by Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

static void fetch_and_display(const struct device *sensor)
{
	static unsigned int count;
	struct sensor_value mag;
	int rc = sensor_sample_fetch(sensor);

	++count;
	if (rc == 0) {
		rc = sensor_channel_get(sensor,
					SENSOR_CHAN_PROX,
					&mag);
	}
	if (rc < 0) {
		printf("ERROR: Update failed: %d\n", rc);
	} else {
		printf("#%u @ %u ms: %d\n",
		       count, k_uptime_get_32(), mag.val1);
	}
}

#ifdef CONFIG_SM351LT_TRIGGER
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	fetch_and_display(dev);
}
#endif

void main(void)
{
	const struct device *sensor = device_get_binding(DT_LABEL(DT_INST(0, honeywell_sm351lt)));

	if (sensor == NULL) {
		printf("Could not get %s device\n",
		       DT_LABEL(DT_INST(0, honeywell_sm351lt)));
		return;
	}

#if CONFIG_SM351LT_TRIGGER
	{
		struct sensor_trigger trig;
		int rc;

		trig.type = SENSOR_TRIG_NEAR_FAR;
		trig.chan = SENSOR_CHAN_PROX;

		struct sensor_value trigger_type = {
			.val1 = GPIO_INT_EDGE_BOTH,
		};

		rc = sensor_attr_set(sensor, trig.chan,
				     SENSOR_ATTR_PRIV_START,
				     &trigger_type);
		if (rc != 0) {
			printf("Failed to set trigger type: %d\n", rc);
			return;
		}

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
#else /* CONFIG_SM351LT_TRIGGER */
	printf("Polling at 0.5 Hz\n");
	while (true) {
		fetch_and_display(sensor);
		k_sleep(K_MSEC(2000));
	}
#endif /* CONFIG_SM351LT_TRIGGER */
}
