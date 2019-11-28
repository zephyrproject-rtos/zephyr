/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/__assert.h>

K_SEM_DEFINE(sem, 0, 1);

static void trigger_handler(struct device *dev, struct sensor_trigger *trigger)
{
	k_sem_give(&sem);
}

static int sensor_set_attribute(struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, int value)
{
	struct sensor_value sensor_val;
	int ret;

	sensor_val.val1 = value / 1000000;
	sensor_val.val2 = value % 1000000;

	ret = sensor_attr_set(dev, chan, attr, &sensor_val);
	if (ret) {
		printf("sensor_attr_set failed ret %d\n", ret);
	}

	return ret;
}

static void process(struct device *dev)
{
	struct sensor_value temp_val;
	int ret;

	/* Set upddate rate to 240 mHz */
	sensor_set_attribute(dev, SENSOR_CHAN_AMBIENT_TEMP,
			     SENSOR_ATTR_SAMPLING_FREQUENCY, 240 * 1000);

	if (IS_ENABLED(CONFIG_ADT7420_TRIGGER)) {
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_THRESHOLD,
			.chan = SENSOR_CHAN_AMBIENT_TEMP,
		};

		/* Set lower and upper threshold to 10 and 30 °C */
		sensor_set_attribute(dev, SENSOR_CHAN_AMBIENT_TEMP,
				     SENSOR_ATTR_UPPER_THRESH, 30 * 1000000);
		sensor_set_attribute(dev, SENSOR_CHAN_AMBIENT_TEMP,
				     SENSOR_ATTR_LOWER_THRESH, 10 * 1000000);

		if (sensor_trigger_set(dev, &trig, trigger_handler)) {
			printf("Could not set trigger\n");
			return;
		}
	}

	while (1) {
		if (IS_ENABLED(CONFIG_ADT7420_TRIGGER)) {
			printf("Waiting for a threshold event\n");
			k_sem_take(&sem, K_FOREVER);
		}

		ret = sensor_sample_fetch(dev);
		if (ret) {
			printf("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
					 &temp_val);
		if (ret) {
			printf("sensor_channel_get failed ret %d\n", ret);
			return;
		}

		printf("temperature %.6f C\n",
		       sensor_value_to_double(&temp_val));

		if (!IS_ENABLED(CONFIG_ADT7420_TRIGGER)) {
			k_sleep(K_MSEC(1000));
		}
	}
}

void main(void)
{
	struct device *dev = device_get_binding(DT_INST_0_ADI_ADT7420_LABEL);

	if (dev == NULL) {
		printf("Failed to get device binding\n");
		return;
	}

	printf("device is %p, name is %s\n", dev, dev->config->name);

	process(dev);
}
