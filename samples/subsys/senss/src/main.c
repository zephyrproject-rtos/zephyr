/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <senss/senss.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int data_event_callback(
		senss_sensor_handle_t handle,
		void *buf, int size, void *param)
{
	const struct senss_sensor_info *info = senss_get_sensor_info(handle);

	printf("App [%s]->\n", __func__);

	printf("Received sensor data from sensor: %s, type: 0x%x, sensor index: %d\n",
		info->name,
		info->type,
		info->sensor_index
	);

	if (info->type == SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER_3D) {
		struct senss_sensor_value_3d_int32* value = buf;
		printf("Received sensor data readings : %d\n",
			value->header.reading_count);
		for (int i = 0; i < value->header.reading_count; i++) {
			printf("Accel sensor data : x: %d, y: %d, z: %d\n",
				value->readings[i].x,
				value->readings[i].y,
				value->readings[i].z);
		}
	} else if (info->type == SENSS_SENSOR_TYPE_MOTION_HINGE_ANGLE) {
		struct senss_sensor_value_int32* value = buf;
		printf("Received sensor data readings : %d\n",
			value->header.reading_count);
		for (int i = 0; i < value->header.reading_count; i++) {
			printf("hinge sensor data : %d\n",
				value->readings[i].v);
		}
	}

	printf("App [%s]<-\n", __func__);

	return 0;
}

void main(void)
{
	int ret;
	ret = senss_init();
	if (ret != 0 ) {
		printf("Failed to init sensor subsystem! \n");
		return;
	}

	/* Enumerate all sensor instances */
	int sensor_count;
	sensor_count = senss_get_sensor_count(SENSS_SENSOR_TYPE_ALL, -1);

	if (sensor_count > 0 ) {
		struct senss_sensor_info *sensor_info =
			malloc(sensor_count * sizeof(struct senss_sensor_info));

		ret = senss_find_sensor(SENSS_SENSOR_TYPE_ALL, -1,
			sensor_info, sensor_count);
		if (ret != 0) {
			for ( int i = 0; i < ret; i++ ) {
				printf("Find sensor: name : %s, type: 0x%x\n",
					sensor_info[i].name,
					sensor_info[i].type);
			}
		}
	}

	/* Open accelerometer instance with sensor index 0 and receive streaming data */
	senss_sensor_handle_t acc_handle;
	ret = senss_open_sensor(SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER_3D,
		0, &acc_handle);
	if (!ret) {
		senss_register_data_event_callback(acc_handle,
			data_event_callback, NULL);

		/* Set 100 Hz frequency */
		senss_set_interval(acc_handle, 10 * USEC_PER_MSEC);

		/* Set streaming mode */
		senss_set_sensitivity(acc_handle, -1, 0);
	}

	senss_sensor_handle_t hinge_handle;
	ret = senss_open_sensor(SENSS_SENSOR_TYPE_MOTION_HINGE_ANGLE,
		0, &hinge_handle);
	if (!ret) {
		senss_register_data_event_callback(hinge_handle,
			data_event_callback, NULL);

		/* Set 100 Hz frequency */
		senss_set_interval(hinge_handle, 10 * USEC_PER_MSEC);

		/* Set streaming mode */
		senss_set_sensitivity(hinge_handle, -1, 0);
	}

	while (true) {
		k_sleep(K_SECONDS(10));
	}

}
