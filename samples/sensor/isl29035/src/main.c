/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

#define LUX_ALERT_DELTA 50

static volatile bool alerted;
struct k_sem sem;

static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
#ifdef CONFIG_ISL29035_TRIGGER
	alerted = !alerted;
	k_sem_give(&sem);
#endif /* CONFIG_ISL29035_TRIGGER */
}

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
		 h, min, s, ms);
	return buf;
}

static void process_sample(const struct device *dev)
{
	static bool last_alerted;
	struct sensor_value val;

	if (sensor_sample_fetch(dev) < 0) {
		printf("Sensor sample update error\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &val) < 0) {
		printf("Cannot read ISL29035 value\n");
		return;
	}

	int lux = val.val1;

	if (IS_ENABLED(CONFIG_ISL29035_TRIGGER)
	    && (alerted != last_alerted)) {
		static int last_lux;
		int rc;
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_THRESHOLD,
			.chan = SENSOR_CHAN_ALL,
		};
		struct sensor_value lo_thr = { MAX(lux - LUX_ALERT_DELTA, 0), };
		struct sensor_value hi_thr = { lux + LUX_ALERT_DELTA };

		printf("ALERT %d lux outside range centered on %d lux."
		       "\nNext alert outside %d .. %d\n",
		       lux, last_lux, lo_thr.val1, hi_thr.val1);
		last_lux = lux;
		last_alerted = alerted;

		rc = sensor_attr_set(dev, SENSOR_CHAN_LIGHT,
				     SENSOR_ATTR_LOWER_THRESH, &lo_thr);
		if (rc == 0) {
			rc = sensor_attr_set(dev, SENSOR_CHAN_LIGHT,
					     SENSOR_ATTR_UPPER_THRESH, &hi_thr);
		}
		if (rc == 0) {
			rc = sensor_trigger_set(dev, &trig, trigger_handler);
		}
		if (rc != 0) {
			printf("Alert configuration failed: %d\n", rc);
		}
	}

	printf("[%s] %s: %g\n", now_str(),
	       IS_ENABLED(CONFIG_ISL29035_MODE_ALS)
	       ? "Ambient light sense"
	       : "IR sense",
	       sensor_value_to_double(&val));
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(isil_isl29035);

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	k_sem_init(&sem, 0, 1);
	alerted = true;
	while (true) {
		process_sample(dev);

		if (IS_ENABLED(CONFIG_ISL29035_TRIGGER)) {
			k_sem_take(&sem, K_SECONDS(10));
		} else {
			k_sleep(K_SECONDS(1));
		}
	}
	return 0;
}
