/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_common, CONFIG_STEPPER_LOG_LEVEL);

#define STEPPER_COMMON_CONFIG_FROM_DEV(dev)                                                        \
	(((const struct stepper_common_config_base *)dev->config)->config)
#define STEPPER_COMMON_DATA_FROM_DEV(dev) (((struct stepper_common_data_base *)dev->data)->data)

int stepper_common_enable(const struct device *dev)
{
	const struct stepper_common_config *config = STEPPER_COMMON_CONFIG_FROM_DEV(dev);

	if (config->enable == NULL) {
		LOG_ERR("Device does not support enabling");
		return -ENOTSUP;
	}

	return config->enable(dev);
}

int stepper_common_disable(const struct device *dev)
{
	const struct stepper_common_config *config = STEPPER_COMMON_CONFIG_FROM_DEV(dev);

	if (config->disable == NULL) {
		LOG_ERR("Device does not support disabling");
		return -ENOTSUP;
	}

	return config->disable(dev);
}

int stepper_common_move_to(const struct device *dev, const int32_t position)
{
	const struct stepper_common_data *data = STEPPER_COMMON_DATA_FROM_DEV(dev);

	return stepper_motion_controller_move_by(dev, position - data->position);
}

int stepper_common_move_by(const struct device *dev, const int32_t micro_steps)
{
	const struct stepper_common_config *config = STEPPER_COMMON_CONFIG_FROM_DEV(dev);

	if (config->step == NULL || config->set_direction == NULL) {
		LOG_ERR("Device does not support moving");
		return -ENOTSUP;
	}

	return stepper_motion_controller_move_by(dev, micro_steps);
}

int stepper_common_step(const struct device *dev)
{
	const struct stepper_common_config *config = STEPPER_COMMON_CONFIG_FROM_DEV(dev);

	if (config->step == NULL) {
		LOG_ERR("Device does not support stepping");
		return -ENOTSUP;
	}

	struct stepper_common_data *data = STEPPER_COMMON_DATA_FROM_DEV(dev);
	int ret = 0;

	ret = config->step(dev);
	if (ret < 0) {
		LOG_ERR("Failed to step: %d", ret);
		return ret;
	}

	data->position += data->direction;
	return 0;
}

int stepper_common_set_direction(const struct device *dev, const enum stepper_direction direction)
{
	const struct stepper_common_config *config = STEPPER_COMMON_CONFIG_FROM_DEV(dev);

	if (config->set_direction == NULL) {
		LOG_ERR("Device does not support setting direction");
		return -ENOTSUP;
	}

	struct stepper_common_data *data = STEPPER_COMMON_DATA_FROM_DEV(dev);
	int ret = 0;

	ret = config->set_direction(dev, direction);
	if (ret < 0) {
		LOG_ERR("Failed to set direction: %d", ret);
		return ret;
	}

	data->direction = direction;

	return 0;
}

int stepper_common_is_moving(const struct device *dev, bool *is_moving)
{
	return stepper_motion_controller_is_moving(dev, is_moving);
}

int stepper_common_set_event_callback(const struct device *dev,
				      const stepper_event_callback_t callback, void *user_data)
{
	struct stepper_common_data *data = STEPPER_COMMON_DATA_FROM_DEV(dev);

	data->event_callback = callback;
	data->event_callback_user_data = user_data;

	return 0;
}

int stepper_common_set_position(const struct device *dev, const int32_t position)
{
	struct stepper_common_data *data = STEPPER_COMMON_DATA_FROM_DEV(dev);

	data->position = position;
	return 0;
}

int stepper_common_get_position(const struct device *dev, int32_t *position)
{
	const struct stepper_common_data *data = STEPPER_COMMON_DATA_FROM_DEV(dev);
	*position = data->position;
	return 0;
}

int stepper_common_run(const struct device *dev, const enum stepper_direction direction)
{
	const struct stepper_common_config *config = STEPPER_COMMON_CONFIG_FROM_DEV(dev);

	if (config->step == NULL || config->set_direction == NULL) {
		LOG_ERR("Device does not support running");
		return -ENOTSUP;
	}

	const int32_t steps = (direction == STEPPER_DIRECTION_POSITIVE) ? INT32_MAX : INT32_MIN;

	return stepper_motion_controller_move_by(dev, steps);
}

int stepper_common_stop(const struct device *dev)
{
	const struct stepper_common_config *config = STEPPER_COMMON_CONFIG_FROM_DEV(dev);

	if (config->step == NULL) {
		LOG_ERR("Device does not support stopping");
		return -ENOTSUP;
	}

	return stepper_motion_controller_stop(dev);
}
int stepper_common_set_ramp(const struct device *dev,
			    const struct stepper_ramp_profile *ramp_profile)
{
	return stepper_motion_controller_set_ramp(dev, ramp_profile);
}

void stepper_common_handle_event(const struct device *dev, const enum stepper_event event)
{
	const struct stepper_common_data *data = STEPPER_COMMON_DATA_FROM_DEV(dev);

	data->event_callback(dev, event, data->event_callback_user_data);
}
