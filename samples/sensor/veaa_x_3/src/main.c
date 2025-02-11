/*
 * Copyright (c) 2024 Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/veaa_x_3.h>

static const struct device *const dev = DEVICE_DT_GET_ONE(festo_veaa_x_3);

int main(void)
{
	int rc;
	struct sensor_value range, setpoint, pressure;

	printk("Testing %s\n", dev->name);

	if (!device_is_ready(dev)) {
		printk("%s not ready\n", dev->name);
		return -ENODEV;
	}

	rc = sensor_attr_get(dev, SENSOR_CHAN_PRESS,
			     (enum sensor_attribute)SENSOR_ATTR_VEAA_X_3_RANGE, &range);
	if (rc != 0) {
		printk("get range failed: %d\n", rc);
		return rc;
	}
	printk("Valve range: %u to %u kPa\n", range.val1, range.val2);

	if (IS_ENABLED(CONFIG_SAMPLE_USE_SHELL)) {
		printk("Loop is disabled. Use the `sensor` command to test %s", dev->name);
		return 0;
	}

	setpoint.val1 = range.val1;
	while (1) {
		rc = sensor_attr_set(dev, SENSOR_CHAN_PRESS,
				     (enum sensor_attribute)SENSOR_ATTR_VEAA_X_3_SETPOINT,
				     &setpoint);
		if (rc != 0) {
			printk("Set setpoint to %u failed: %d\n", setpoint.val1, rc);
		}

		/* Sleep before get to allow DAC and ADC to stabilize */
		k_msleep(CONFIG_SAMPLE_LOOP_INTERVAL);

		rc = sensor_sample_fetch(dev);
		if (rc != 0) {
			printk("Fetch sample failed: %d", rc);
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);
		if (rc != 0) {
			printk("Get sample failed: %d", rc);
		}

		printk("Setpoint: %4u kPa, actual: %4u kPa\n", setpoint.val1, pressure.val1);

		setpoint.val1 += CONFIG_SAMPLE_LOOP_INCREMENT;
		if (setpoint.val1 > range.val2) {
			setpoint.val1 = range.val1;
		}
	}

	return 0;
}
