/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SAMPLING_FREQ 25

void print_distance_and_prox(const struct device *dev)
{
	struct sensor_value value;
	int ret;

	ret = sensor_sample_fetch(dev);
	if (ret) {
		printf("sensor_sample_fetch failed ret %d\n", ret);
		return;
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &value);
	printf("prox is %d\n", value.val1);

	ret = sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &value);
	printf("distance is %.3fm\n", sensor_value_to_double(&value));
}

void on_new_data_ready(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(trig);
	print_distance_and_prox(dev);
}

void main(void)
{
	int ret;
	bool trigger_enabled = false;
	const struct device *const dev = DEVICE_DT_GET_ONE(st_vl53l0x);
	const struct sensor_trigger trigger = {
		.chan = SENSOR_CHAN_DISTANCE,
		.type = SENSOR_TRIG_DATA_READY,
	};
	const struct sensor_value sampling_freq = {
		.val1 = SAMPLING_FREQ,
		.val2 = 0,
	};

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return;
	}

	ret = sensor_attr_set(dev, SENSOR_CHAN_DISTANCE, SENSOR_ATTR_SAMPLING_FREQUENCY,
			      &sampling_freq);
	if (ret) {
		printf("Unable to set sampling freq to %dHz\n", SAMPLING_FREQ);
		return;
	}

	ret = sensor_trigger_set(dev, &trigger, on_new_data_ready);
	switch (ret) {
	case 0:
		trigger_enabled = true;
		printf("Trigger enabled\n");
		break;
	case -ENOTSUP:
		printf("Sensor does not support trigger\n");
		break;
	default:
		return;
	}

	while (1) {
		if (!trigger_enabled) {
			print_distance_and_prox(dev);
		}
		k_sleep(K_MSEC(1000));
	}
}
