/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>
#include <sys/__assert.h>

void main(void)
{
	const struct device *dev;
	struct sensor_value temp_value;

	/* offset to be added to the temperature
	 * only supported by TMP117
	 */
	struct sensor_value offset_value;
	int ret;

	dev = device_get_binding(DT_LABEL(DT_INST(0, ti_tmp116)));
	__ASSERT(dev != NULL, "Failed to get TMP116 device binding");

	printk("Device %s - %p is ready\n", dev->name, dev);

	/*
	 * if an offset of 2.5 oC is to be added,
	 * set val1 = 2 and val2 = 500000.
	 * See struct sensor_value documentation for more details.
	 */
	offset_value.val1 = 0;
	offset_value.val2 = 0;
	ret = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
			      SENSOR_ATTR_OFFSET, &offset_value);
	if (ret) {
		printk("sensor_attr_set failed ret = %d\n", ret);
		printk("SENSOR_ATTR_OFFSET is only supported by TMP117\n");
	}
	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("Failed to fetch measurements (%d)\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
					 &temp_value);
		if (ret) {
			printk("Failed to get measurements (%d)\n", ret);
			return;
		}

		printk("temp is %d.%d oC\n", temp_value.val1, temp_value.val2);

		k_sleep(K_MSEC(1000));
	}
}
