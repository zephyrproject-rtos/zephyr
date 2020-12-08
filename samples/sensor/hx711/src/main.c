/*
 * Copyright (c) 2020 Kalyan Sriram <kalyan@coderkalyan.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>

#define ADC_NODE DT_INST(0, avia_hx711)

#if DT_NODE_HAS_STATUS(ADC_NODE, okay)
#define ADC_LABEL DT_LABEL(ADC_NODE)
#else
#error "Unsupported board."
#endif

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	ms = now % MSEC_PER_SEC;
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

static int process_hx711(const struct device *dev)
{
	struct sensor_value load_calib;
	struct sensor_value load_raw;
	int rc = sensor_sample_fetch(dev);

	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_FORCE, &load_calib);
	}
	if (rc == 0) {
		rc = sensor_channel_get(dev, SENSOR_CHAN_PRIV_START, &load_raw);
	}

	if (rc == 0) {
		printf("[%s]:\n"
			   "calibrated: %f grams\n"
			   "raw: %f grams\n",
			   now_str(),
			   sensor_value_to_double(&load_calib),
			   sensor_value_to_double(&load_raw));
	} else {
		printf("sample fetch/get failed: %d\n", rc);
	}

	return rc;
}

#ifdef CONFIG_HX711_TRIGGER
static struct sensor_trigger trigger;

static void handle_hx711_trigger(const struct device *dev,
		struct sensor_trigger *trig)
{
	int rc = process_hx711(dev);

	if (rc != 0) {
		printk("Cancelling trigger due to failure: %d\n", rc);
		(void) sensor_trigger_set(dev, trig, NULL);
		return;
	}
}
#endif /* CONFIG_HX711_TRIGGER */

void main(void)
{
	const struct device *hx711 = device_get_binding(ADC_LABEL);

	if (hx711 == NULL) {
		printk("Unable to find device %s\n", ADC_LABEL);
		return;
	}

#ifdef CONFIG_HX711_TRIGGER
	trigger = (struct sensor_trigger) {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	if (sensor_trigger_set(hx711, &trigger,
				handle_hx711_trigger) < 0) {
		printk("Cannot configure trigger\n");
		return;
	}
	printk("Configured for triggered sampling");
#endif

	while (!IS_ENABLED(CONFIG_HX711_TRIGGER)) {
		int rc = process_hx711(hx711);

		if (rc != 0) {
			return;
		}

		k_sleep(K_SECONDS(2));
	}

	/* triggered runs with its own thread after exit */
}
