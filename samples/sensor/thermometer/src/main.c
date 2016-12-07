/*
 * Copyright (c) 2016 ARM Ltd.
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
#include <sensor.h>
#include <stdio.h>

static double convert_to_double(struct sensor_value *v)
{
	switch (v->type) {
	case SENSOR_VALUE_TYPE_INT:
		return v->val1;
	case SENSOR_VALUE_TYPE_INT_PLUS_MICRO:
		return v->val1 + ((double)v->val2 / 1000000);
	case SENSOR_VALUE_TYPE_DOUBLE:
		return v->dval;
	}
	return 0;
}

void main(void)
{
	struct device *temp_dev;

	printf("Thermometer Example! %s\n", CONFIG_ARCH);

	temp_dev = device_get_binding("TEMP_0");
	if (!temp_dev) {
		printf("error: no temp device\n");
		return;
	}

	printf("temp device is %p, name is %s\n",
	       temp_dev, temp_dev->config->name);

	while (1) {
		int r;
		struct sensor_value temp_value;

		r = sensor_sample_fetch(temp_dev);
		if (r) {
			printf("sensor_sample_fetch failed return: %d\n", r);
			break;
		}

		r = sensor_channel_get(temp_dev, SENSOR_CHAN_TEMP,
				       &temp_value);
		if (r) {
			printf("sensor_channel_get failed return: %d\n", r);
			break;
		}

		printf("Temperature is %gC\n", convert_to_double(&temp_value));

		k_sleep(1000);
	}
}
