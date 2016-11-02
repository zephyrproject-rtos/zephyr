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
#include <misc/printk.h>
#include <sensor.h>

/**
 * @file Sample app using the MAX44009 light sensor through ARC I2C.
 *
 * This app will read the sensor reading via I2C interface and show
 * the result every 4 seconds.
 */

void main(void)
{
	struct device *dev;
	struct sensor_value val;
	uint32_t lum = 0;

	printk("MAX44009 light sensor application\n");

	dev = device_get_binding("MAX44009");
	if (!dev) {
		printk("sensor: device not found.\n");
		return;
	}

	while (1) {
		if (sensor_sample_fetch_chan(dev, SENSOR_CHAN_LIGHT) != 0) {
			printk("sensor: sample fetch fail.\n");
			return;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &val) != 0) {
			printk("sensor: channel get fail.\n");
			return;
		}

		lum = val.dval;
		printk("sensor: lum reading: %d\n", lum);

		k_sleep(4000);
	}
}
