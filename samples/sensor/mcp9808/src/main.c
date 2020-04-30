/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>

#define UCEL_PER_CEL 1000000
#define UCEL_PER_MCEL 1000
#define TEMP_INITIAL_CEL 25
#define TEMP_WINDOW_HALF_UCEL 500000

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

#ifdef CONFIG_MCP9808_TRIGGER

static struct sensor_trigger trig;

static int set_window(const struct device *dev,
		      const struct sensor_value *temp)
{
	const int temp_ucel = temp->val1 * UCEL_PER_CEL + temp->val2;
	const int low_ucel = temp_ucel - TEMP_WINDOW_HALF_UCEL;
	const int high_ucel = temp_ucel + TEMP_WINDOW_HALF_UCEL;
	struct sensor_value val = {
		.val1 = low_ucel / UCEL_PER_CEL,
		.val2 = low_ucel % UCEL_PER_CEL,
	};
	int rc = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
				 SENSOR_ATTR_LOWER_THRESH, &val);
	if (rc == 0) {
		val.val1 = high_ucel / UCEL_PER_CEL,
		val.val2 = high_ucel % UCEL_PER_CEL,
		rc = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
				     SENSOR_ATTR_UPPER_THRESH, &val);
	}

	if (rc == 0) {
		printf("Alert on temp outside [%d, %d] milli-Celsius\n",
		       low_ucel / UCEL_PER_MCEL,
		       high_ucel / UCEL_PER_MCEL);
	}

	return rc;
}

static inline int set_window_ucel(const struct device *dev,
				  int temp_ucel)
{
	struct sensor_value val = {
		.val1 = temp_ucel / UCEL_PER_CEL,
		.val2 = temp_ucel % UCEL_PER_CEL,
	};

	return set_window(dev, &val);
}

static void trigger_handler(const struct device *dev,
			    struct sensor_trigger *trig)
{
	struct sensor_value temp;
	static size_t cnt;

	++cnt;
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);

	printf("trigger fired %u, temp %g deg C\n", cnt,
	       sensor_value_to_double(&temp));
	set_window(dev, &temp);
}
#endif

void main(void)
{
	const char *const devname = DT_LABEL(DT_INST(0, microchip_mcp9808));
	const struct device *dev = device_get_binding(devname);
	int rc;

	if (dev == NULL) {
		printf("Device %s not found.\n", devname);
		return;
	}

#ifdef CONFIG_MCP9808_TRIGGER
	rc = set_window_ucel(dev, TEMP_INITIAL_CEL * UCEL_PER_CEL);
	if (rc == 0) {
		trig.type = SENSOR_TRIG_THRESHOLD;
		trig.chan = SENSOR_CHAN_AMBIENT_TEMP;
		rc = sensor_trigger_set(dev, &trig, trigger_handler);
	}

	if (rc != 0) {
		printf("Trigger set failed: %d\n", rc);
		return;
	}
	printk("Trigger set got %d\n", rc);
#endif

	while (1) {
		struct sensor_value temp;

		rc = sensor_sample_fetch(dev);
		if (rc != 0) {
			printf("sensor_sample_fetch error: %d\n", rc);
			break;
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		if (rc != 0) {
			printf("sensor_channel_get error: %d\n", rc);
			break;
		}

		printf("%s: %g C\n", now_str(),
		       sensor_value_to_double(&temp));

		k_sleep(K_SECONDS(2));
	}
}
