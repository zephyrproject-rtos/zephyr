/*
 * Copyright (c) 2016 Intel Corporation
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

#ifdef CONFIG_SX9500_TRIGGER

static void sensor_trigger_handler(struct device *dev, struct sensor_trigger *trig)
{
	struct sensor_value prox_value;
	int ret;

	ret = sensor_sample_fetch(dev);
	if (ret) {
		printk("sensor_sample_fetch failed ret %d\n", ret);
		return;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &prox_value);
	printk("prox is %d\n", prox_value.val1);
}

static void setup_trigger(struct device *dev)
{
	int ret;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_NEAR_FAR,
	};

	ret = sensor_trigger_set(dev, &trig, sensor_trigger_handler);
	if (ret) {
		printk("sensor_trigger_set err %d\n", ret);
	}
}

void do_main(struct device *dev)
{
	setup_trigger(dev);

	while (1) {
		k_sleep(1000);
	}
}

#else /* CONFIG_SX9500_TRIGGER */

static void do_main(struct device *dev)
{
	int ret;
	struct sensor_value prox_value;

	while (1) {
		ret = sensor_sample_fetch(dev);
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &prox_value);
		printk("prox is %d\n", prox_value.val1);

		k_sleep(1000);
	}
}

#endif /* CONFIG_SX9500_TRIGGER */

void main(void)
{
	struct device *dev;

	dev = device_get_binding("SX9500");
	printk("device is %p, name is %s\n", dev, dev->config->name);

	do_main(dev);
}
