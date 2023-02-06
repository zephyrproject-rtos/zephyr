/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensing_sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(sensing, CONFIG_SENSING_LOG_LEVEL);

/* sensing_open_sensor is normally called by applications: hid, chre, zephyr main, etc */
int sensing_open_sensor(const struct sensing_sensor_info *info,
			const struct sensing_callback_list *cb_list,
			sensing_sensor_handle_t *handle)
{
	return -ENOTSUP;
}

int sensing_open_sensor_by_dt(const struct device *dev,
			      const struct sensing_callback_list *cb_list,
			      sensing_sensor_handle_t *handle)
{
	return -ENOTSUP;
}

/* sensing_close_sensor is normally called by applications: hid, chre, zephyr main, etc */
int sensing_close_sensor(sensing_sensor_handle_t handle)
{
	return -ENOTSUP;
}

int sensing_set_config(sensing_sensor_handle_t handle,
		       struct sensing_sensor_config *configs,
		       int count)
{
	return -ENOTSUP;
}

int sensing_get_config(sensing_sensor_handle_t handle,
		       struct sensing_sensor_config *configs,
		       int count)
{
	return -ENOTSUP;
}

const struct sensing_sensor_info *sensing_get_sensor_info(sensing_sensor_handle_t handle)
{
	return NULL;
}
