/*
 * Copyright (c) 2020 arithmetics.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/fdc2x1x.h>
#include <stdio.h>

#define CH_BUF_INIT(m)          {},

K_SEM_DEFINE(sem, 0, 1);

#ifdef CONFIG_FDC2X1X_TRIGGER
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trigger)
{
	switch (trigger->type) {
	case SENSOR_TRIG_DATA_READY:
		if (sensor_sample_fetch(dev)) {
			printk("Sample fetch error\n");
			return;
		}
		k_sem_give(&sem);
		break;
	default:
		printk("Unknown trigger\n");
	}
}
#endif

#ifdef CONFIG_PM_DEVICE
static void pm_info(enum pm_device_action action, int status)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		printk("Enter ACTIVE_STATE ");
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		printk("Enter SUSPEND_STATE ");
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		printk("Enter OFF_STATE ");
		break;
	default:
		printk("Unknown power state");
	}

	if (status) {
		printk("Fail\n");
	} else {
		printk("Success\n");
	}
}
#endif

#define DEVICE_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(ti_fdc2x1x)

void main(void)
{
	struct sensor_value ch_buf[] = {
		DT_FOREACH_CHILD(DEVICE_NODE, CH_BUF_INIT)
	};

	uint8_t num_ch = ARRAY_SIZE(ch_buf);
	enum sensor_channel base;
	int i;

	const struct device *dev = DEVICE_DT_GET(DEVICE_NODE);

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return;
	}

#ifdef CONFIG_FDC2X1X_TRIGGER
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printk("Could not set trigger\n");
		return;
	}
#endif

#ifdef CONFIG_PM_DEVICE
	/* Testing the power modes */
	enum pm_device_action p_action;
	int ret;

	p_action = PM_DEVICE_ACTION_SUSPEND;
	ret = pm_device_action_run(dev, p_action);
	pm_info(p_action, ret);

	p_action = PM_DEVICE_ACTION_TURN_OFF;
	ret = pm_device_action_run(dev, p_action);
	pm_info(p_action, ret);

	p_action = PM_DEVICE_ACTION_RESUME;
	ret = pm_device_action_run(dev, p_action);
	pm_info(p_action, ret);
#endif

	while (1) {
#ifdef CONFIG_FDC2X1X_TRIGGER
		k_sem_take(&sem, K_FOREVER);
#else
		if (sensor_sample_fetch(dev) < 0) {
			printk("Sample fetch failed\n");
			return;
		}
#endif
		base = SENSOR_CHAN_FDC2X1X_FREQ_CH0;
		for (i = 0; i < num_ch; i++) {
			sensor_channel_get(dev, base++, &ch_buf[i]);
			printf("ch%d: %f MHz ", i, sensor_value_to_double(&ch_buf[i]));
		}
		printf("\n");

		base = SENSOR_CHAN_FDC2X1X_CAPACITANCE_CH0;
		for (i = 0; i < num_ch; i++) {
			sensor_channel_get(dev, base++, &ch_buf[i]);
			printf("ch%d: %f pF ", i, sensor_value_to_double(&ch_buf[i]));
		}
		printf("\n\n");


#ifdef CONFIG_PM_DEVICE
		p_action = PM_DEVICE_ACTION_TURN_OFF;
		ret = pm_device_action_run(dev, p_action);
		pm_info(p_action, ret);
		k_sleep(K_MSEC(2000));
		p_action = PM_DEVICE_ACTION_RESUME;
		ret = pm_device_action_run(dev, p_action);
		pm_info(p_action, ret);
#elif CONFIG_FDC2X1X_TRIGGER_NONE
		k_sleep(K_MSEC(100));
#endif
	}
}
