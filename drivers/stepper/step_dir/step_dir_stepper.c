/*
 * Copyright 2025 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "step_dir_stepper.h"

/* Allows wrapper functions to access the step_dir_stepper_api struct for function pointers. */
struct step_dir_stepper_wrapper_config {
	union step_dir_stepper_config config;
	const struct step_dir_stepper_api *api;
};

int step_dir_stepper_move_by(const struct device *dev, const int32_t micro_steps)
{
	const struct step_dir_stepper_wrapper_config *config = dev->config;

	return config->api->move_by(dev, micro_steps);
}

int step_dir_stepper_set_microstep_interval(const struct device *dev,
					    const uint64_t microstep_interval_ns)
{
	const struct step_dir_stepper_wrapper_config *config = dev->config;

	return config->api->set_microstep_interval(dev, microstep_interval_ns);
}

int step_dir_stepper_set_reference_position(const struct device *dev, const int32_t value)
{
	const struct step_dir_stepper_wrapper_config *config = dev->config;

	return config->api->set_reference_position(dev, value);
}

int step_dir_stepper_get_actual_position(const struct device *dev, int32_t *value)
{
	const struct step_dir_stepper_wrapper_config *config = dev->config;

	return config->api->get_actual_position(dev, value);
}

int step_dir_stepper_move_to(const struct device *dev, const int32_t value)
{
	const struct step_dir_stepper_wrapper_config *config = dev->config;

	return config->api->move_to(dev, value);
}

int step_dir_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	const struct step_dir_stepper_wrapper_config *config = dev->config;

	return config->api->is_moving(dev, is_moving);
}

int step_dir_stepper_run(const struct device *dev, const enum stepper_direction direction)
{
	const struct step_dir_stepper_wrapper_config *config = dev->config;

	return config->api->run(dev, direction);
}

int step_dir_stepper_stop(const struct device *dev)
{
	const struct step_dir_stepper_wrapper_config *config = dev->config;

	return config->api->stop(dev);
}

int step_dir_stepper_set_event_callback(const struct device *dev, stepper_event_callback_t callback,
					void *user_data)
{
	const struct step_dir_stepper_wrapper_config *config = dev->config;

	return config->api->set_event_callback(dev, callback, user_data);
}
