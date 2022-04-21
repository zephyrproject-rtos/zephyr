/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>

void main(void)
{
	struct sensor_value accel[3];

	const struct device *dev = DEVICE_DT_GET_ANY(adi_adxl345);

	if (dev == NULL) {
		printk("device_get_binding() failed");
		return;
	}

	while (true) {
		int ret;
		static const enum sensor_channel channels[] = {
			SENSOR_CHAN_ACCEL_X,
			SENSOR_CHAN_ACCEL_Y,
			SENSOR_CHAN_ACCEL_Z,
		};

		ret = sensor_sample_fetch(dev);
		if (ret < 0) {
			printk("sensor_Sample_fetch() failed: %d\n", ret);
			return;
		}

		for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
			ret = sensor_channel_get(dev, channels[i], &accel[i]);
			if (ret < 0) {
				printk("sensor_channel_get(%d) failed: %d\n", channels[i], ret);
				return;
			}
		}

		printk("accel x:%d.%06d y:%d.%06d z:%d.%06d (m/s^2)\n",
		       accel[0].val1, accel[0].val2,
		       accel[1].val1, accel[1].val2,
		       accel[2].val1, accel[2].val2);

		k_sleep(K_MSEC(1000));
	}
}
