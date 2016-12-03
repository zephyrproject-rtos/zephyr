/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
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

static void trigger_handler(struct device *dev, struct sensor_trigger *trigger)
{
	struct sensor_value accel[3];
	struct sensor_value magn[3];

	sensor_sample_fetch(dev);

	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_ANY, accel);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_ANY, magn);

	/* Print accel x,y,z and mag x,y,z data */
	printf("AX=%3d.%06d AY=%3d.%06d AZ=%3d.%06d "
	       "MX=%3d.%06d MY=%3d.%06d MZ=%3d.%06d\n",
		accel[0].val1, accel[0].val2,
		accel[1].val1, accel[1].val2,
		accel[2].val1, accel[2].val2,
		magn[0].val1, magn[0].val2,
		magn[1].val1, magn[1].val2,
		magn[2].val1, magn[2].val2
	      );
}

void main(void)
{
	struct device *dev = device_get_binding(CONFIG_FXOS8700_NAME);

	if (dev == NULL) {
		printf("Could not get fxos8700 device\n");
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_ANY,
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger\n");
		return;
	}
}
