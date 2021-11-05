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

	/* example of how you can call function in the driver from main.
	If you need to pass parameters use val1 and val2.
	SENSOR_ATTR_BQ35100_EXAMPLE1 is just as an example. You could create
	e.g. SENSOR_ATTR_BQ35100_GAUGE_START to start the gauge and so on. */
	/*val.val1 = 1;
	val.val2 = 2;
	sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_BQ35100_EXAMPLE1, &val);*/


	while (1) {
		if (sensor_sample_fetch(dev) < 0) {
			printk("Sample fetch failed\n");
			return;
		}

		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_TEMP, &val);
		printf("Temperature:    %2.1f  C\n", sensor_value_to_double(&val));

		sensor_channel_get(dev, SENSOR_CHAN_BQ35100_GAUGE_INT_TEMP, &val);
		printf("Internal Temp:  %2.1f  C\n", sensor_value_to_double(&val));
		
		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_VOLTAGE, &val);
		printf("Voltage:        %1.3f V\n", sensor_value_to_double(&val));
	
		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_AVG_CURRENT, &val);
		printf("Current:        %i   mA\n", val.val1);

		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_STATE_OF_HEALTH, &val);
		printf("State of Health:%i   %%\n", val.val1);

		sensor_channel_get(dev, SENSON_CHAN_BQ35100_GAUGE_DES_CAP, &val);
		printf("Design Capacity:%i  mAh\n", val.val1);

		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_ACCUMULATED_CAPACITY, &val);
		printk("Acc Capacity :  %i  uAh\n\n", val.val1);

		k_sleep(K_MSEC(3000));
	}
}