/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/math/util.h>
#include <zephyr/drivers/sensor_types.h>
#include <zephyr/drivers/sensor.h>
#include <icm42688.h> /* for debugging */

#include <stdint.h>
#include <stdio.h>

K_SEM_DEFINE(data_sem, 0, K_SEM_MAX_LIMIT);

/* TODO Should provide void* userdata, ideally provides an event not data (e.g. step, tap, tilt, drdy, fifo full, fifo wm, etc)? */
int data_callback(const struct device *icm42688, struct sensor_raw_data *data)
{
	/* TODO How long does data live? Can I pass it to another thread? Unclear, or I have no control */
	k_sem_give(&data_sem);

	struct sensor_fifo_iterator_api iter_api;
		
	/* TODO iterator is valid for how long? Unclear from this API, no way of checking validity */
	if (sensor_get_fifo_iterator_api(icm42688, &iter_api) != 0) {
		printf("Trouble getting sensor iterator\n");
		return -EINVAL;
	}
	
	struct sensor_fifo_iterator iter = SENSOR_FIFO_ITERATOR_OF(data);

	uint32_t sensor_type;
	struct sensor_three_axis_data icm42688_accel;
	struct sensor_three_axis_data icm42688_gyro;
	struct sensor_float_data icm42688_temp;
	

	/* TODO no API function pointer wrappers, would be nice */
	/* TODO How many samples are here? The iterator only has next */
	/* TODO Do I need next first or is the first value already here, unclear from docs, not only changes
	 * the loop required by also means the first item must be valid if next does not start before the first
	 * reading
	 */
	/* TODO which sensor reading is the current one in terms of sample index? Totally unclear */

	while (iter_api.next(&iter)) {
		if (iter_api.get_sensor_type(&iter, &sensor_type) != 0) {
			printf("Trouble getting sensor type from iterator\n");
		};
		
		if (sensor_type == SENSOR_TYPE_ACCELEROMETER) {
			if (iter_api.read(&iter, &icm42688_accel) != 0) {
				printf("Trouble reading sensor type accelerometer\n");
			}
		} else if (sensor_type == SENSOR_TYPE_GYROSCOPE) {
			if (iter_api.read(&iter, &icm42688_gyro) != 0) {
				printf("Trouble reading sensor type gyroscope\n");
			}
		} else if (sensor_type == SENSOR_TYPE_ACCELEROMETER_TEMPERATURE 
			   || sensor_type == SENSOR_TYPE_GYROSCOPE_TEMPERATURE) {
			if (iter_api.read(&iter, &icm42688_temp) != 0) {
				printf("Trouble reading sensor type temp");
			}
		}
	}
		
	/* TODO Unclear what happens if I return -errno from the docs */
	return 0;
}

void main(void)
{
	printk("TDK RoboKit1 Sample \n");

	const struct device *icm42688 = DEVICE_DT_GET_ONE(invensense_icm42688);

	if (!device_is_ready(icm42688)) {
		printk("%s: device not ready.\n", icm42688->name);
		return;
	}
	
	struct sensor_three_axis_data icm42688_accel;
	struct sensor_three_axis_data icm42688_gyro;
	struct sensor_float_data icm42688_temp;
	uint8_t data[14];
		
	/* 10 seconds of polling mode */
	for (int i = 0; i < 100; i++) {
		/* TODO how would I avoid these 3 calls? */
		if(sensor_read_data(icm42688, SENSOR_TYPE_ACCELEROMETER, &icm42688_accel) != 0) {
			printf("Trouble reading accelerometer data from %s\n", icm42688->name);
		}

		if(sensor_read_data(icm42688, SENSOR_TYPE_GYROSCOPE, &icm42688_gyro) != 0) {
			printf("Trouble reading gyroscope data from %s\n", icm42688->name);
		}		/* Display sensor data */
		if(sensor_read_data(icm42688, SENSOR_TYPE_ACCELEROMETER_TEMPERATURE, &icm42688_temp) != 0) {
			printf("Trouble reading temp data from %s\n", icm42688->name);
		}
		if(icm42688_read_all(icm42688, data) != 0) {
			printf("Trouble reading raw data from %s\n", icm42688->name);
		}

		/* Erase previous */
		printf("\0033\014");

		printf("TDK RoboKit1 dashboard (Polling Mode)\n\n");

		printf("ICM42688: Accel (m/s^2): x: %" PRIf ", y: %" PRIf ", z: %" PRIf "\n",
			PRIf_ARG(icm42688_accel.readings[0].x),
			PRIf_ARG(icm42688_accel.readings[0].y),
			PRIf_ARG(icm42688_accel.readings[0].z));
		printf("ICM42688: Gyro (rad/s): x: %" PRIf ", y: %" PRIf ", z: %" PRIf "\n",
			PRIf_ARG(icm42688_gyro.readings[0].x),
			PRIf_ARG(icm42688_gyro.readings[0].y),
			PRIf_ARG(icm42688_gyro.readings[0].z));
		printf("ICM42688: Temp (C): %" PRIf "\n",
			PRIf_ARG(icm42688_temp.readings[0].value));

		icm42688_read_all_dbg(&((struct icm42688_dev_data *)icm42688->data)->cfg, data);

		k_sleep(K_MSEC(100));
	}
	/* 10 seconds of FIFO mode */

	
	if (sensor_fifo_set_watermark(icm42688, 50, true) != 0) {
		printf("Trouble setting fifo watermark\n");
	}
	
	/* What context does the callback run in? Can I pass userdata? */
	if (sensor_set_process_data_callback(dev, data_callback) != 0) {
		printf("Trouble setting process callback\n");
	}
	
	if (sensor_fifo_set_streaming_mode(icm42688, true) != 0) {
		printf("Trouble setting streaming mode\n");
	}
	
	/* How would I get my data into this thread easily? */
	for (int i = 0; i < 100; i++) {
		k_sem_take(&data_sem, K_FOREVER);
		printf("Got fifo data from callback, iter %d\n", i);
	}
		
	printk("Exiting!\n");
}
