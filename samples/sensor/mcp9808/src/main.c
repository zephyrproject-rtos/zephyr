/*
 * Copyright (c) 2016 Intel Corporation.
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
#include <nanokernel.h>
#include <misc/printk.h>

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <misc/sys_log.h>

#ifdef CONFIG_MCP9808_TRIGGER
static void trigger_handler(struct device *dev, struct sensor_trigger *trig)
{
	struct sensor_value temp;

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_TEMP, &temp);

	SYS_LOG_INF("trigger fired, temp %d.%06d", temp.val1, temp.val2);
}
#endif

void main(void)
{
	struct device *dev = device_get_binding("MCP9808");

	if (dev == NULL) {
		SYS_LOG_ERR("device not found.  aborting test.");
		return;
	}

	SYS_LOG_DBG("dev %p", dev);
	SYS_LOG_DBG("dev %p name %s", dev, dev->config->name);

#ifdef CONFIG_MCP9808_TRIGGER
	struct sensor_value val;
	struct sensor_trigger trig;

	val.type = SENSOR_VALUE_TYPE_INT;
	val.val1 = 26;

	sensor_attr_set(dev, SENSOR_CHAN_TEMP,
			SENSOR_ATTR_UPPER_THRESH, &val);

	trig.type = SENSOR_TRIG_THRESHOLD;
	trig.chan = SENSOR_CHAN_TEMP;

	sensor_trigger_set(dev, &trig, trigger_handler);
#endif

	while (1) {
		struct sensor_value temp;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_TEMP, &temp);

		SYS_LOG_INF("temp: %d.%06d", temp.val1, temp.val2);

		task_sleep(sys_clock_ticks_per_sec);
	}
}
