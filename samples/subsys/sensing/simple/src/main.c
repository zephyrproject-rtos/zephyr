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

static void hinge_angle_data_event_callback(sensing_sensor_handle_t handle, const void *buf,
				    void *context)
{
	const struct sensing_sensor_info *info = sensing_get_sensor_info(handle);
	struct sensing_sensor_value_q31 *sample = (struct sensing_sensor_value_q31 *)buf;

	ARG_UNUSED(context);

	LOG_INF("handle:%p, Sensor:%s data:(v:%d)", handle, info->name, sample->readings[0].v);
}

int main(void)
{
	const struct sensing_callback_list base_acc_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_callback_list lid_acc_cb_list = {
		.on_data_event = &acc_data_event_callback,
	};
	const struct sensing_callback_list hinge_angle_cb_list = {
		.on_data_event = &hinge_angle_data_event_callback,
	};
	const struct sensing_sensor_info *info;
	sensing_sensor_handle_t base_acc;
	sensing_sensor_handle_t lid_acc;
	sensing_sensor_handle_t hinge_angle;
	struct sensing_sensor_config base_acc_config;
	struct sensing_sensor_config lid_acc_config;
	struct sensing_sensor_config hinge_angle_config;
	const struct sensing_sensor_info *tmp_sensor_info;
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
		LOG_ERR("sensing_open_sensor_by_dt, type:0x%x index:1 error:%d",
			SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D, ret);
	}
	ret = sensing_open_sensor_by_dt(DEVICE_DT_GET(DT_NODELABEL(hinge_angle)),
					&hinge_angle_cb_list,
					&hinge_angle);
	if (ret) {
		LOG_ERR("sensing_open_sensor_by_type, type:0x%x index:0 error:%d",
			SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE, ret);
	}

	/* set base acc, lid acc, hinge sensor interval */
	base_acc_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	base_acc_config.interval = 100 * USEC_PER_MSEC;
	ret = sensing_set_config(base_acc, &base_acc_config, 1);
	if (ret) {
		LOG_ERR("base_acc sensing_set_interval error:%d\n", ret);
	}

	lid_acc_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	lid_acc_config.interval = 100 * USEC_PER_MSEC;
	ret = sensing_set_config(lid_acc, &lid_acc_config, 1);
	if (ret) {
		LOG_ERR("lid_acc sensing_set_interval error:%d\n", ret);
	}

	tmp_sensor_info = sensing_get_sensor_info(hinge_angle);
	hinge_angle_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	hinge_angle_config.interval = tmp_sensor_info->minimal_interval;
	ret = sensing_set_config(hinge_angle, &hinge_angle_config, 1);
	if (ret) {
		LOG_ERR("hinge_angle sensing_set_interval error:%d\n", ret);
	}

	memset(&base_acc_config, 0x00, sizeof(struct sensing_sensor_config));
	memset(&lid_acc_config, 0x00, sizeof(struct sensing_sensor_config));
	memset(&hinge_angle_config, 0x00, sizeof(struct sensing_sensor_config));

	/* get base acc, lid acc, hinge sensor interval */
	base_acc_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	ret = sensing_get_config(base_acc, &base_acc_config, 1);
	if (ret) {
		LOG_ERR("base_acc sensing_get_interval error:%d\n", ret);
	}

	lid_acc_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	ret = sensing_get_config(lid_acc, &lid_acc_config, 1);
	if (ret) {
		LOG_ERR("lid_acc sensing_get_interval error:%d\n", ret);
	}

	hinge_angle_config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
	ret = sensing_get_config(hinge_angle, &hinge_angle_config, 1);
	if (ret) {
		LOG_ERR("hinge_angle sensing_get_interval error:%d\n", ret);
	}

	/* set base acc, lid acc, hinge sensor sensitivity */
	base_acc_config.attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	base_acc_config.data_field = SENSING_SENSITIVITY_INDEX_ALL;
	base_acc_config.sensitivity = 0;
	ret = sensing_set_config(base_acc, &base_acc_config, 1);
	if (ret) {
		LOG_ERR("base_acc sensing_set_sensitivity error:%d\n", ret);
	}

	lid_acc_config.attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	lid_acc_config.data_field = SENSING_SENSITIVITY_INDEX_ALL;
	lid_acc_config.sensitivity = 0;
	ret = sensing_set_config(lid_acc, &lid_acc_config, 1);
	if (ret) {
		LOG_ERR("lid_acc sensing_set_sensitivity error:%d\n", ret);
	}

	hinge_angle_config.attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	hinge_angle_config.data_field = SENSING_SENSITIVITY_INDEX_ALL;
	hinge_angle_config.sensitivity = 1;
	ret = sensing_set_config(hinge_angle, &hinge_angle_config, 1);
	if (ret) {
		LOG_ERR("hinge_angle sensing_set_sensitivity error:%d\n", ret);
	}

	memset(&base_acc_config, 0x00, sizeof(struct sensing_sensor_config));
	memset(&lid_acc_config, 0x00, sizeof(struct sensing_sensor_config));
	memset(&hinge_angle_config, 0x00, sizeof(struct sensing_sensor_config));

	/* get base acc, lid acc, hinge sensor sensitivity */
	base_acc_config.attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	base_acc_config.data_field = SENSING_SENSITIVITY_INDEX_ALL;
	ret = sensing_get_config(base_acc, &base_acc_config, 1);
	if (ret) {
		LOG_ERR("base_acc sensing_get_sensitivity error:%d\n", ret);
	}

	lid_acc_config.attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	lid_acc_config.data_field = SENSING_SENSITIVITY_INDEX_ALL;
	ret = sensing_get_config(lid_acc, &lid_acc_config, 1);
	if (ret) {
		LOG_ERR("lid_acc sensing_get_sensitivity error:%d\n", ret);
	}

	hinge_angle_config.attri = SENSING_SENSOR_ATTRIBUTE_SENSITIVITY;
	hinge_angle_config.data_field = SENSING_SENSITIVITY_INDEX_ALL;
	ret = sensing_get_config(hinge_angle, &hinge_angle_config, 1);
	if (ret) {
		LOG_ERR("hinge_angle sensing_get_sensitivity error:%d\n", ret);
	}

	ret = sensing_close_sensor(&lid_acc);
	if (ret) {
		LOG_ERR("sensing_close_sensor:%p error:%d", lid_acc, ret);
	}

	return 0;
}
