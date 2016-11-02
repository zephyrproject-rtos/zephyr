/*
 * Copyright (c) 2016 Firmwave
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <misc/printk.h>
#include <misc/__assert.h>

static void do_main(struct device *dev)
{
	int ret;
	struct sensor_value temp_value;
	struct sensor_value attr;

	attr.type = SENSOR_VALUE_TYPE_INT;
	attr.val1 = 150;
	ret = sensor_attr_set(dev, SENSOR_CHAN_TEMP,
			      SENSOR_ATTR_FULL_SCALE, &attr);
	if (ret) {
		printk("sensor_attr_set failed ret %d\n", ret);
		return;
	}

	attr.type = SENSOR_VALUE_TYPE_INT_PLUS_MICRO;
	attr.val1 = 8;
	attr.val2 = 0;
	ret = sensor_attr_set(dev, SENSOR_CHAN_TEMP,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);
	if (ret) {
		printk("sensor_attr_set failed ret %d\n", ret);
		return;
	}

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_TEMP, &temp_value);
		if (ret) {
			printk("sensor_channel_get failed ret %d\n", ret);
			return;
		}

		printk("temp is %d (%d micro)\n", temp_value.val1,
		       temp_value.val2);

		k_sleep(1000);
	}
}

void main(void)
{
	struct device *dev;

	dev = device_get_binding("TMP112");
	__ASSERT(dev != NULL, "Failed to get device binding");
	printk("device is %p, name is %s\n", dev, dev->config->name);

	do_main(dev);
}
