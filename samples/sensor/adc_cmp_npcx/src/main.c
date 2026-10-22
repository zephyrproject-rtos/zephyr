/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adc_cmp_npcx.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#define ADC_CMP_NODE			DT_NODELABEL(adc_cmp)

#define ADC_CMP_UPPER_THRESHOLD_MV	1000
#define ADC_CMP_LOWER_THRESHOLD_MV	250
#define IS_RUNNING			!(atomic_test_bit(&stop, 0))
#define STOP()				atomic_set_bit(&stop, 0)

enum threshold_state {
	THRESHOLD_UPPER,
	THRESHOLD_LOWER
} state;

atomic_val_t stop;

const struct sensor_trigger sensor_trig = {
	.type = SENSOR_TRIG_THRESHOLD,
	.chan = SENSOR_CHAN_VOLTAGE
};

void set_upper_threshold(const struct device *dev)
{
	struct sensor_value val;
	int err;

	printf("ADC CMP: Set Upper threshold");
	val.val1 = ADC_CMP_UPPER_THRESHOLD_MV;
	err = sensor_attr_set(dev, SENSOR_CHAN_VOLTAGE,
		SENSOR_ATTR_UPPER_VOLTAGE_THRESH, &val);
	if (err) {
		printf("Error setting upper threshold");
		STOP();
	}
}

void set_lower_threshold(const struct device *dev)
{
	struct sensor_value val;
	int err;

	printf("ADC CMP: Set Lower threshold");
	val.val1 = ADC_CMP_LOWER_THRESHOLD_MV;
	err = sensor_attr_set(dev, SENSOR_CHAN_VOLTAGE,
		SENSOR_ATTR_LOWER_VOLTAGE_THRESH, &val);
	if (err) {
		printf("ADC CMP: Error setting lower threshold");
		STOP();
	}
}

void enable_threshold(const struct device *dev, bool enable)
{
	struct sensor_value val;
	int err;

	val.val1 = enable;
	err = sensor_attr_set(dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_ALERT,
			&val);
	if (err) {
		printf("ADC CMP: Error %sabling threshold",
			enable ? "en":"dis");
		STOP();
	}
}

void threshold_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trigger)
{
	enable_threshold(dev, false);

	if (state == THRESHOLD_UPPER) {
		state = THRESHOLD_LOWER;
		printf("ADC CMP: Lower threshold detected");
		set_lower_threshold(dev);
	} else if (state == THRESHOLD_LOWER) {
		state = THRESHOLD_UPPER;
		printf("ADC CMP: Upper threshold detected");
		set_upper_threshold(dev);
	} else {
		printf("ADC CMP: Error, unexpected state");
		STOP();
	}

	enable_threshold(dev, true);
}

int main(void)
{
	const struct device *const adc_cmp = DEVICE_DT_GET(ADC_CMP_NODE);
	int err;

	if (!device_is_ready(adc_cmp)) {
		printf("ADC CMP: Error, device is not ready");
		return 0;
	}

	err = sensor_trigger_set(adc_cmp, &sensor_trig,
		threshold_trigger_handler);
	if (err) {
		printf("ADC CMP: Error setting handler");
		return 0;
	}

	set_upper_threshold(adc_cmp);

	enable_threshold(adc_cmp, true);

	while (IS_RUNNING) {
		k_sleep(K_MSEC(1));
	}

	printf("ADC CMP: Exiting application");
	return 0;
}
