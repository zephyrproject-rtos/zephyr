/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stepper_motion_controller.h"

#include "../stepper_common.h"

#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_motion_controller, CONFIG_STEPPER_LOG_LEVEL);

#define SIGN(x) ((x) < 0 ? -1 : 1)

#define STEPPER_COMMON_FROM_DEV(dev) ((const struct stepper_common_config_base *)dev->config)
#define STEPPER_CONFIG_FROM_DEV(dev) (STEPPER_COMMON_FROM_DEV(dev)->config)
#define CONTROLLER_FROM_DEV(dev)                                                                   \
	((const struct stepper_motion_controller *)STEPPER_CONFIG_FROM_DEV(dev)->motion_controller)

static int stepper_motion_controller_set_direction(const struct device *dev,
						   const enum stepper_direction direction)
{
	const struct stepper_motion_controller *controller = CONTROLLER_FROM_DEV(dev);
	const struct stepper_motion_controller_config *config = controller->config;
	struct stepper_motion_controller_data *data = controller->data;

	K_SPINLOCK(&data->lock) {
		data->direction = direction;
	}

	return config->callbacks->set_direction(dev, direction);
}

static void stepper_motion_controller_calculate_next_interval(const struct device *dev)
{
	const struct stepper_motion_controller *controller = CONTROLLER_FROM_DEV(dev);
	const struct stepper_motion_controller_config *config = controller->config;
	struct stepper_motion_controller_data *data = controller->data;

	uint64_t next_interval = stepper_ramp_get_next_interval(data->ramp);

	if (data->relative_target_position == 0) {
		LOG_DBG("Reached target position");
	}

	if (next_interval > 0) {
		/* the next interval is greater zero, movement is not finished yet */
		K_SPINLOCK(&data->lock) {
			const int ret = config->timing_source_api->start(data->timing_source_data,
									 next_interval);

			if (ret < 0) {
				LOG_ERR("Failed to start timing source: %d", ret);
			}
		}
	} else {
		/* the next interval is zero, movement has finished */
		K_SPINLOCK(&data->lock) {
			int ret = config->timing_source_api->stop(data->timing_source_data);

			if (ret < 0) {
				LOG_ERR("Failed to stop timing source: %d", ret);
			}

			if (data->relative_target_position != 0) {
				stepper_motion_controller_set_direction(
					dev, SIGN(data->relative_target_position));

				stepper_ramp_prepare_move(data->ramp,
							  abs(data->relative_target_position));

				next_interval = stepper_ramp_get_next_interval(data->ramp);

				ret = config->timing_source_api->start(data->timing_source_data,
								       next_interval);

				if (ret < 0) {
					LOG_ERR("Failed to start timing source: %d", ret);
				}
			} else {
				LOG_DBG("Motion completed");

				config->callbacks->event(dev, STEPPER_EVENT_STEPS_COMPLETED);
			}
		}
	}
}

static int stepper_motion_controller_perform_step(const struct device *dev)
{
	const struct stepper_motion_controller *controller = CONTROLLER_FROM_DEV(dev);
	struct stepper_motion_controller_data *data = controller->data;
	int ret = 0;

	ret = controller->config->callbacks->step(dev);
	if (ret < 0) {
		LOG_ERR("Failed to step: %d", ret);
		return ret;
	}

	if (data->relative_target_position != INT32_MAX &&
	    data->relative_target_position != INT_MIN) {
		data->relative_target_position -= data->direction;
	}

	return 0;
}

void stepper_handle_timing_signal(const void *user_data)
{
	const struct device *dev = user_data;

	const int ret = stepper_motion_controller_perform_step(dev);

	if (ret < 0) {
		LOG_ERR("Failed to perform step: %d", ret);
	}

	stepper_motion_controller_calculate_next_interval(dev);
}

int stepper_motion_controller_init(const struct device *dev)
{
	const struct stepper_motion_controller *controller = CONTROLLER_FROM_DEV(dev);
	const struct stepper_motion_controller_config *config = controller->config;
	int ret = 0;

	config->timing_source->callback = &stepper_handle_timing_signal;
	config->timing_source->user_data = dev;

	stepper_motion_controller_set_direction(dev, STEPPER_DIRECTION_POSITIVE);

	ret = config->timing_source_api->init(config->timing_source);
	if (ret < 0) {
		LOG_ERR("Failed to initialize timing source: %d", ret);
		return ret;
	}

	return 0;
}

int stepper_motion_controller_move_by(const struct device *dev, const int32_t micro_steps)
{
	const struct stepper_motion_controller *controller = CONTROLLER_FROM_DEV(dev);
	struct stepper_motion_controller_data *data = controller->data;
	const struct stepper_motion_controller_config *config = controller->config;

	if (data->ramp == NULL) {
		LOG_ERR("Ramp is not set");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		LOG_DBG("Moving by %d microsteps", micro_steps);

		uint64_t movement_steps_count = 0;

		const bool is_moving =
			config->timing_source_api->get_interval(config->timing_source) > 0;
		const bool is_moving_in_same_direction = data->direction == SIGN(micro_steps);

		if (is_moving && !is_moving_in_same_direction) {
			/*
			 * the stepper is moving in the opposite direction, we need to stop the
			 * stepper and after that initiate a movement to the target position
			 */
			movement_steps_count = stepper_ramp_prepare_stop(data->ramp);
		} else {
			stepper_motion_controller_set_direction(dev, SIGN(micro_steps));
			movement_steps_count =
				stepper_ramp_prepare_move(data->ramp, abs(micro_steps));
		}

		data->relative_target_position = micro_steps;

		LOG_DBG("Movement steps count: %llu", movement_steps_count);

		if (movement_steps_count > 0) {
			stepper_motion_controller_calculate_next_interval(dev);
		} else {
			LOG_DBG("Motion completed");
			config->callbacks->event(dev, STEPPER_EVENT_STEPS_COMPLETED);
		}
	}

	return 0;
}

int stepper_motion_controller_is_moving(const struct device *dev, bool *is_moving)
{
	const struct stepper_motion_controller *controller = CONTROLLER_FROM_DEV(dev);
	const struct stepper_motion_controller_data *data = controller->data;

	*is_moving = data->relative_target_position != 0;
	return 0;
}

int stepper_motion_controller_set_ramp(const struct device *dev, struct stepper_ramp_base *ramp)
{
	const struct stepper_motion_controller *controller = CONTROLLER_FROM_DEV(dev);
	struct stepper_motion_controller_data *data = controller->data;

	K_SPINLOCK(&data->lock) {
		data->ramp = ramp;
	}

	return 0;
}

int stepper_motion_controller_stop(const struct device *dev)
{
	const struct stepper_motion_controller *controller = CONTROLLER_FROM_DEV(dev);
	const struct stepper_motion_controller_config *config = controller->config;
	struct stepper_motion_controller_data *data = controller->data;

	const uint64_t stop_steps_count = stepper_ramp_prepare_stop(data->ramp);

	if (stop_steps_count > 0) {
		data->relative_target_position = SIGN(data->direction) * stop_steps_count;

		stepper_motion_controller_calculate_next_interval(dev);
	} else {
		data->relative_target_position = 0;
		const int ret = config->timing_source_api->stop(data->timing_source_data);

		if (ret < 0) {
			LOG_ERR("Failed to stop timing source: %d", ret);
			return ret;
		}
	}

	return 0;
}
