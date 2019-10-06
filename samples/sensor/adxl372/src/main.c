/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/sensor.h>
#include <stdio.h>

#define pow2(x) ((x) * (x))

static double sqrt(double value)
{
	int i;
	double sqrt = value / 3;

	if (value <= 0) {
		return 0;
	}

	for (i = 0; i < 6; i++) {
		sqrt = (sqrt + value / sqrt) / 2;
	}

	return sqrt;
}

K_SEM_DEFINE(sem, 0, 1);

static void trigger_handler(struct device *dev, struct sensor_trigger *trigger)
{
	ARG_UNUSED(trigger);

	if (sensor_sample_fetch(dev)) {
		printf("sensor_sample_fetch failed\n");
		return;
	}

	k_sem_give(&sem);
}

void main(void)
{
	struct sensor_value accel[3];
	double mag;
	int i;
	char meter[200];

	struct device *dev = device_get_binding(DT_INST_0_ADI_ADXL372_LABEL);

	if (dev == NULL) {
		printf("Could not get %s device\n", DT_INST_0_ADI_ADXL372_LABEL);
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (IS_ENABLED(CONFIG_ADXL372_PEAK_DETECT_MODE)) {
		trig.type = SENSOR_TRIG_THRESHOLD;
	}

	if (IS_ENABLED(CONFIG_ADXL372_TRIGGER)) {
		if (sensor_trigger_set(dev, &trig, trigger_handler)) {
			printf("Could not set trigger\n");
			return;
		}
	}

	while (1) {
		if (IS_ENABLED(CONFIG_ADXL372_TRIGGER)) {
			if (IS_ENABLED(CONFIG_ADXL372_PEAK_DETECT_MODE)) {
				printf("Waiting for a threshold event\n");
			}
			k_sem_take(&sem, K_FOREVER);
		} else {
			if (sensor_sample_fetch(dev)) {
				printf("sensor_sample_fetch failed\n");
			}
		}

		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);

		if (IS_ENABLED(CONFIG_ADXL372_PEAK_DETECT_MODE)) {
			mag = sqrt(pow2(sensor_ms2_to_g(&accel[0])) +
				pow2(sensor_ms2_to_g(&accel[1])) +
				pow2(sensor_ms2_to_g(&accel[2])));

			for (i = 0; i <= mag && i < (sizeof(meter) - 1); i++) {
				meter[i] = '#';
			}

			meter[i] = '\0';

			printf("%6.2f g: %s\n", mag, meter);
		} else {
			printf("AX=%10.2f AY=%10.2f AZ=%10.2f (m/s^2)\n",
				sensor_value_to_double(&accel[0]),
				sensor_value_to_double(&accel[1]),
				sensor_value_to_double(&accel[2]));
		}

		if (!IS_ENABLED(CONFIG_ADXL372_TRIGGER)) {
			k_sleep(K_MSEC(2000));
		}
	}
}
