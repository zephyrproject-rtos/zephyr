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

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

void main(void)
{
	struct device *dev = device_get_binding("MCP9808");

	if (dev == NULL) {
		printk("device not found.  aborting test.\n");
		return;
	}

#ifdef DEBUG
	PRINT("dev %p\n", dev);
	PRINT("dev %p name %s\n", dev, dev->config->name);
#endif

	while (1) {
		struct sensor_value temp;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_TEMP, &temp);

		PRINT("temp: %d.%06d\n", temp.val1, temp.val2);

		task_sleep(sys_clock_ticks_per_sec);
	}
}

