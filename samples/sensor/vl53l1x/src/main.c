/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <misc/printk.h>

static void process_sample(struct device *dev)
{
	int ret;
	struct sensor_value value;

	ret = sensor_sample_fetch(dev);

	if (ret) {
		printk("sensor_sample_fetch failed ret %d\n", ret);
		return;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &value);
	printk("prox is %d\n", value.val1);

	ret = sensor_channel_get(dev,
					SENSOR_CHAN_DISTANCE,
					&value);
	printk("distance is %d.%06dm\n", value.val1, value.val2);
}

static void vl53l1x_handler(struct device *dev,
			   struct sensor_trigger *trig)
{
	process_sample(dev);
}


void main(void)
{
	struct device *dev = device_get_binding(DT_ST_VL53L1X_0_LABEL);

	if (dev == NULL) {
		printk("Could not get VL53L1X device\n");
		return;
	}

	if (IS_ENABLED(CONFIG_VL53L1X_TRIGGER)) {
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ALL,
		};
		if (sensor_trigger_set(dev, &trig, vl53l1x_handler) < 0) {
			printk("Cannot configure trigger\n");
			return;
		};
	}

	while (!IS_ENABLED(CONFIG_VL53L1X_TRIGGER)) {
		process_sample(dev);
		k_sleep(1000);
	}
	k_sleep(K_FOREVER);
}
