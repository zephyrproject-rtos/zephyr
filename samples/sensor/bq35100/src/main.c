/*
 * Copyright (c) 2021 arithmetics.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/util.h>
#include <drivers/sensor.h>
#include "drivers/sensor/bq35100.h"
#include <stdio.h>


#ifdef CONFIG_PM_DEVICE
static void pm_info(enum pm_device_state state, int status)
{
	switch (state) {
	case PM_DEVICE_STATE_ACTIVE:
		printk("Enter ACTIVE_STATE ");
		break;
	case PM_DEVICE_STATE_OFF:
		printk("Enter OFF_STATE ");
		break;
	}

	if (status) {
		printk("Fail\n");
	} else {
		printk("Success\n");
	}
}
#endif

#define DEVICE_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(ti_bq35100)

void main(void)
{
	struct sensor_value val;

	const struct device *dev = DEVICE_DT_GET(DEVICE_NODE);

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return;
	}

#ifdef CONFIG_PM_DEVICE
	/* Testing the power modes */
	enum pm_device_state p_state;
	int ret;

	p_state = PM_DEVICE_STATE_OFF;
	ret = pm_device_state_set(dev, p_state);
	pm_info(p_state, ret);
	k_sleep(K_MSEC(1000));

	p_state = PM_DEVICE_STATE_ACTIVE;
	ret = pm_device_state_set(dev, p_state);
	pm_info(p_state, ret);
#endif


	while (1) {
		if (sensor_sample_fetch(dev) < 0) {
			printk("Sample fetch failed\n");
			return;
		}

		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_TEMP, &val);
		printf("Temperature:   %i.%i  C\n", val.val1, val.val2);
		
		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_VOLTAGE, &val);
		printf("Voltage:       %i.%i V\n", val.val1, val.val2);

		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_AVG_CURRENT, &val);
		printf("Current:       %i     A\n", val.val1);

		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &val);
		printf("StateOfHealth: %i     %%\n\n", val.val1);

		k_sleep(K_MSEC(3000));
	}
}
