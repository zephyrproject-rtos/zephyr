/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #include <zephyr/drivers/sensor/max30011.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/max30210.h>
#include <zephyr/drivers/i2c.h>

#define MAX30210_NODE    DT_NODELABEL(max30210)
#define TRIGGER_SET_TEST 1

static const struct device *max30210_dev = DEVICE_DT_GET(MAX30210_NODE);

void handler(const struct device *dev, const struct sensor_trigger *trig)
{
	struct sensor_value temp_value;
	printf("Interrupt triggered: %s\n", dev->name);
	int ret = sensor_sample_fetch(dev);
	if (ret < 0) {
		printf("Failed to fetch sample: %d\n", ret);
		return;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_value);
	if (ret < 0) {
		printf("Failed to get temperature channel: %d\n", ret);
		return;
	}
	uint8_t interrupt_status = 0;
	// ret = max30210_reg_read(dev, 0x00, &interrupt_status, 1); // Read interrupt status
	// register
	printf("Temperature is: %f C \n", sensor_value_to_float(&temp_value));
}

int main(void)
{
	printf("Starting MAX30210 sample application\n");
	if (!device_is_ready(max30210_dev)) {
		printf("MAX30210 device not ready\n");
		return -ENODEV;
	}
	printf("MAX30210 device is ready: %s\n", max30210_dev->name);

	struct sensor_value temp_sampling_freq = {
		.val1 = 2, // Set your desired sampling frequency in Hz
		.val2 = 0};

	int ret = sensor_attr_set(max30210_dev, SENSOR_CHAN_AMBIENT_TEMP,
				  SENSOR_ATTR_MAX30210_CONTINUOUS_CONVERSION_MODE, NULL);
	ret = sensor_attr_set(max30210_dev, SENSOR_CHAN_AMBIENT_TEMP,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &temp_sampling_freq);

#if 0
	while (1){
	
	struct sensor_value temp_value;
	ret = sensor_sample_fetch(max30210_dev);
	if (ret < 0) {
		printf("Failed to fetch sample: %d\n", ret);
		return ret;
	}
	ret = sensor_channel_get(max30210_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_value);
	if (ret < 0) {
		printf("Failed to get temperature channel: %d\n", ret);
		return ret;
	}
	printf("Temperature is: %f C \n", sensor_value_to_float(&temp_value));
	}
	return 0;
#endif

	struct sensor_trigger trig = {.type = SENSOR_TRIG_THRESHOLD,
				      .chan = SENSOR_CHAN_AMBIENT_TEMP};

	ret = sensor_attr_set(max30210_dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_UPPER_THRESH,
			      &(struct sensor_value){.val1 = 28, .val2 = 0});
	if (ret < 0) {
		printf("Failed to set upper threshold: %d\n", ret);
		return ret;
	}
	ret = sensor_attr_set(max30210_dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_LOWER_THRESH,
			      &(struct sensor_value){.val1 = 20, .val2 = 0});
	if (ret < 0) {
		printf("Failed to set lower threshold: %d\n", ret);
		return ret;
	}

	ret = sensor_trigger_set(max30210_dev, &trig, handler);
	if (ret < 0) {
		printf("Failed to set trigger: %d\n", ret);
		return ret;
	}

	uint8_t interrupt_enable_setup = 0x00; // Initialize to 0x00
	ret = max30210_reg_read(max30210_dev, 0x02, &interrupt_enable_setup, 1);

	printf("Interrupt Enable Setup: 0x%02X\n", interrupt_enable_setup);

	// uint8_t alarm_high_setup[2]={0x17, 0x70};
	// // ret = max30210_reg_write(max30210_dev, 0x22, alarm_high_setup[0], 1);
	// // if (ret < 0) {
	// // 	printf("Failed to write TEMP_ALARM_HIGH_SETUP: %d\n", ret);
	// // 	return ret;
	// // }
	// // ret = max30210_reg_write(max30210_dev, 0x23, alarm_high_setup[1], 1);
	// // if (ret < 0) {
	// // 	printf("Failed to write TEMP_ALARM_HIGH_MSB: %d\n", ret);
	// // 	return ret;
	// // }

	// ret = max30210_reg_write_multiple(max30210_dev, 0x22, alarm_high_setup, 2);
	// if (ret < 0) {
	// 	printf("Failed to write TEMP_ALARM_HIGH_SETUP: %d\n", ret);
	// 	return ret;
	// }
	// // ret = max30210_reg_write(max30210_dev, 0x22, alarm_high_setup, 2); // Clear
	// TEMP_ALARM_HIGH_SETUP register
	// // if (ret < 0) {
	// // 	printf("Failed to write TEMP_ALARM_HIGH_SETUP: %d\n", ret);
	// // 	return ret;
	// // }
	// ret = max30210_reg_read(max30210_dev, 0x22, alarm_high_setup, 2);
	// printf("Alarm High Setup: 0x%02X 0x%02X\n", alarm_high_setup[0], alarm_high_setup[1]);

	while (1) {
		struct sensor_value temp_value;
		ret = sensor_sample_fetch(max30210_dev);
		if (ret < 0) {
			printf("Failed to fetch sample: %d\n", ret);
			return ret;
		}
		ret = sensor_channel_get(max30210_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_value);
		if (ret < 0) {
			printf("Failed to get temperature channel: %d\n", ret);
			return ret;
		}
		printf("Temperature is: %f C \n", sensor_value_to_float(&temp_value));
	}
}
