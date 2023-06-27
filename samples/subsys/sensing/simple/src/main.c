/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sensing/sensing.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static void acc_data_event_callback(sensing_sensor_handle_t handle, const void *buf,
				    void *context)
{
	const struct sensing_sensor_info *info = sensing_get_sensor_info(handle);
	struct sensing_sensor_value_3d_q31 *sample = (struct sensing_sensor_value_3d_q31 *)buf;
	ARG_UNUSED(context);

	LOG_INF("%s(%d), handle:%p, Sensor:%s data:(x:%d, y:%d, z:%d)",
		__func__, __LINE__, handle, info->name,
		sample->readings[0].x,
		sample->readings[0].y,
		sample->readings[0].z);
}

int main(void)
{
	const struct sensing_callback_list base_acc_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_callback_list lid_acc_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t base_acc;
	sensing_sensor_handle_t lid_acc;
	int ret, i, num = 0;

	ret = sensing_get_sensors(&num, &info);
	if (ret) {
		LOG_ERR("sensing_get_sensors error");
		return 0;
	}

	for (i = 0; i < num; ++i) {
		LOG_INF("Sensor %d: name: %s friendly_name: %s type: %d",
			 i,
			 info[i].name,
			 info[i].friendly_name,
			 info[i].type);
	}

	LOG_INF("sensing subsystem run successfully");

	ret = sensing_open_sensor(&info[0], &base_acc_cb_list, &base_acc);
	if (ret) {
		LOG_ERR("sensing_open_sensor, type:0x%x index:0 error:%d",
			SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D, ret);
	}

	ret = sensing_open_sensor_by_dt(DEVICE_DT_GET(DT_NODELABEL(lid_accel)),
					&lid_acc_cb_list,
					&lid_acc);
	if (ret) {
		LOG_ERR("sensing_open_sensor, type:0x%x index:1 error:%d",
			SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D, ret);
	}

	ret = sensing_close_sensor(&base_acc);
	if (ret) {
		LOG_ERR("sensing_close_sensor:%p error:%d", base_acc, ret);
	}

	ret = sensing_close_sensor(&lid_acc);
	if (ret) {
		LOG_ERR("sensing_close_sensor:%p error:%d", lid_acc, ret);
	}
	return 0;
}
