/*
 * Copyright (c) 2022, Luke Holt
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <stdio.h>

#ifdef CONFIG_LTR390_TRIGGER

#define INITIAL_LIGHT_LEVEL     100.0
#define LIGHT_LEVEL_VARIANCE    3

static struct sensor_trigger trigger;

static int set_window(const struct device *dev,
					  enum sensor_channel chan,
					  const double light_level)
{
	int rc;

	struct sensor_value lower, upper;

	rc = sensor_value_from_double(&lower, light_level - LIGHT_LEVEL_VARIANCE);
	if (rc < 0) {
		printf("Could not convert double to value struct");
		return rc;
	}
	rc = sensor_value_from_double(&upper, light_level + LIGHT_LEVEL_VARIANCE);
	if (rc < 0) {
		printf("Could not convert double to value struct");
		return rc;
	}

	rc = sensor_attr_set(dev, chan, SENSOR_ATTR_LOWER_THRESH, &lower);
	if (rc < 0) {
		printf("Could not set lower threshold");
		return rc;
	}

	rc = sensor_attr_set(dev, chan, SENSOR_ATTR_UPPER_THRESH, &upper);
	if (rc < 0) {
		printf("Could not set lower threshold");
		return rc;
	}

	printf("Alert on ambient light [%d.%06d, %d.%06d] lux\n",
		lower.val1, lower.val2, upper.val1, upper.val2);

	return 0;
}

static void trigger_handler(const struct device *dev,
							const struct sensor_trigger *trig)
{
	struct sensor_value val;
	static size_t cnt;
	int rc;

	++cnt;
	rc = sensor_sample_fetch(dev);
	if (rc < 0) {
		printf("sensor_sample_fetch error: %d\n", rc);
		return;
	}

	rc = sensor_channel_get(dev, trig->chan, &val);
	if (rc < 0) {
		printf("sensor_channel_get error: %d\n", rc);
		return;
	}

	printf("Trigger fired %u, val: %d.%06d\n", cnt, val.val1, val.val2);

	set_window(dev, trig->chan, sensor_value_to_double(&val));
}
#endif /* CONFIG_LTR390_TRIGGER */

void main(void)
{
	/*
	 * Get "liteon,ltr390" from DT.
	 */
	const struct device *dev = DEVICE_DT_GET_ANY(liteon_ltr390);
	int rc;

	if (dev == NULL) {
		/* Node not found, or does not have OK status. */
		printk("\nERROR: no device found.\n");
	}

	if (!device_is_ready(dev)) {
		printk("\nERROR: Device\"%s\" is not ready.\n", dev->name);
	}

	printk("Found device \"%s\"\n", dev->name);


#ifdef CONFIG_LTR390_TRIGGER
	rc = set_window(dev, SENSOR_CHAN_LIGHT, INITIAL_LIGHT_LEVEL);
	if (rc < 0) {
		printf("Failed to set window");
	} else {
		trigger.type = SENSOR_TRIG_THRESHOLD;
		trigger.chan = SENSOR_CHAN_LIGHT;
		rc = sensor_trigger_set(dev, &trigger, trigger_handler);
		if (rc < 0) {
			printf("Failed to set trigger");
		}
	}
#endif


	while (1) {
		struct sensor_value light, uvi;

		rc = sensor_sample_fetch(dev);
		if (rc != 0) {
			printf("sensor_sample_fetch error: %d\n", rc);
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &light);
		if (rc != 0) {
			printf("sensor_channel_get error: %d\n", rc);
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_UVI, &uvi);
		if (rc != 0) {
			printf("sensor_channel_get error: %d\n", rc);
		}

		if (rc == 0) {
			printf("Light: \t%d.%06d lux\tUVI: \t%d.%06d\n",
				light.val1, light.val2, uvi.val1, uvi.val2);
		}

		k_sleep(K_MSEC(5000));
	}
}
