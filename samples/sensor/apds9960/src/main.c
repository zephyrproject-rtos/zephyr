/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/sensor.h>
#include <device.h>
#include <stdio.h>
#include <sys/printk.h>

#ifdef CONFIG_APDS9960_TRIGGER
K_SEM_DEFINE(sem, 0, 1);

static void trigger_handler(const struct device *dev,
			    struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trigger);

	k_sem_give(&sem);
}
#endif

void main(void)
{
	const struct device *dev;
	struct sensor_value intensity, pdata;

	printk("APDS9960 sample application\n");
	dev = device_get_binding(DT_LABEL(DT_INST(0, avago_apds9960)));
	if (!dev) {
		printk("sensor: device not found.\n");
		return;
	}

#ifdef CONFIG_APDS9960_TRIGGER
	struct sensor_value attr = {
		.val1 = 127,
		.val2 = 0,
	};

	if (sensor_attr_set(dev, SENSOR_CHAN_PROX,
			    SENSOR_ATTR_UPPER_THRESH, &attr)) {
		printk("Could not set threshold\n");
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_PROX,
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printk("Could not set trigger\n");
		return;
	}
#endif

	while (true) {
#ifdef CONFIG_APDS9960_TRIGGER
		printk("Waiting for a threshold event\n");
		k_sem_take(&sem, K_FOREVER);
#else
		k_sleep(K_MSEC(5000));
#endif
		if (sensor_sample_fetch(dev)) {
			printk("sensor_sample fetch failed\n");
		}

		sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &intensity);
		sensor_channel_get(dev, SENSOR_CHAN_PROX, &pdata);

		printk("ambient light intensity %d, proximity %d\n",
		       intensity.val1, pdata.val1);

#ifdef CONFIG_PM_DEVICE
		uint32_t p_state;

		p_state = DEVICE_PM_LOW_POWER_STATE;
		device_set_power_state(dev, p_state, NULL, NULL);
		printk("set low power state for 2s\n");
		k_sleep(K_MSEC(2000));
		p_state = DEVICE_PM_ACTIVE_STATE;
		device_set_power_state(dev, p_state, NULL, NULL);
#endif
	}
}
