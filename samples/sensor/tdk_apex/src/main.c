/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/tdk_apex.h>
#include <stdio.h>

static struct sensor_trigger data_trigger;

/* Flag set from IMU device irq handler */
static volatile int irq_from_device;

/*
 * Get a device structure from a devicetree node from alias
 * "tdk_apex_sensor0".
 */
static const struct device *get_tdk_apex_device(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(tdk_apex_sensor0));

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;
}

#if defined(CONFIG_TDK_APEX_PEDOMETER) || defined(CONFIG_TDK_APEX_TILT) ||                         \
	defined(CONFIG_TDK_APEX_WOM) || defined(CONFIG_TDK_APEX_SMD)
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

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u", h, min, s, ms);
	return buf;
}
#endif

static void handle_tdk_apex_drdy(const struct device *dev, const struct sensor_trigger *trig)
{
	if (trig->type == SENSOR_TRIG_MOTION) {
		int rc = sensor_sample_fetch_chan(dev, trig->chan);

		if (rc < 0) {
			printf("sample fetch failed: %d\n", rc);
			printf("cancelling trigger due to failure: %d\n", rc);
			(void)sensor_trigger_set(dev, trig, NULL);
			return;
		} else if (rc == 0) {
			irq_from_device = 1;
		}
	}
}

int main(void)
{
	const struct device *dev = get_tdk_apex_device();
	struct sensor_value apex_mode;

	if (dev == NULL) {
		return 0;
	}

	/* Setting APEX Pedometer feature */
#ifdef CONFIG_TDK_APEX_PEDOMETER
	apex_mode.val1 = TDK_APEX_PEDOMETER;
#endif
#ifdef CONFIG_TDK_APEX_TILT
	apex_mode.val1 = TDK_APEX_TILT;
#endif
#ifdef CONFIG_TDK_APEX_WOM
	apex_mode.val1 = TDK_APEX_WOM;
#endif
#ifdef CONFIG_TDK_APEX_SMD
	apex_mode.val1 = TDK_APEX_SMD;
#endif
	apex_mode.val2 = 0;
	sensor_attr_set(dev, SENSOR_CHAN_APEX_MOTION, SENSOR_ATTR_CONFIGURATION, &apex_mode);

	data_trigger = (struct sensor_trigger){
		.type = SENSOR_TRIG_MOTION,
		.chan = SENSOR_CHAN_APEX_MOTION,
	};
	if (sensor_trigger_set(dev, &data_trigger, handle_tdk_apex_drdy) < 0) {
		printf("Cannot configure data trigger!!!\n");
		return 0;
	}

	printf("Configured for APEX data collecting.\n");

	k_sleep(K_MSEC(1000));

	while (1) {

		if (irq_from_device) {
#ifdef CONFIG_TDK_APEX_PEDOMETER
			struct sensor_value apex_pedometer[3];

			sensor_channel_get(dev, SENSOR_CHAN_APEX_MOTION, apex_pedometer);

			printf("[%s]: STEP_DET     count: %d steps  cadence: %.1f steps/s  "
			       "activity: %s\n",
			       now_str(), apex_pedometer[0].val1,
			       sensor_value_to_double(&apex_pedometer[2]),
			       apex_pedometer[1].val1 == 1   ? "Walk"
			       : apex_pedometer[1].val1 == 2 ? "Run"
							     : "Unknown");
#endif
#ifdef CONFIG_TDK_APEX_TILT
			printf("[%s]: TILT\n", now_str());
#endif
#ifdef CONFIG_TDK_APEX_WOM
			struct sensor_value apex_wom[3];

			sensor_channel_get(dev, SENSOR_CHAN_APEX_MOTION, apex_wom);

			printf("[%s]: WOM x=%d y=%d z=%d\n", now_str(), apex_wom[0].val1,
			       apex_wom[1].val1, apex_wom[2].val1);
#endif
#ifdef CONFIG_TDK_APEX_SMD
			printf("[%s]: SMD\n", now_str());
#endif
			irq_from_device = 0;
		}
	}
	return 0;
}
