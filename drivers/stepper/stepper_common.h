/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STEPPER_COMMON_H
#define STEPPER_COMMON_H

#include <zephyr/drivers/stepper.h>

#include "motion_controller/stepper_motion_controller.h"

typedef int (*stepper_common_step_t)(const struct device *);

typedef int (*stepper_common_set_direction_t)(const struct device *, enum stepper_direction);

struct stepper_common_config {

	struct stepper_motion_controller *const motion_controller;

	void *const interface;

	stepper_enable_t enable;

	stepper_disable_t disable;

	stepper_common_step_t step;

	stepper_common_set_direction_t set_direction;
};

struct stepper_common_data {

	int32_t position;

	enum stepper_direction direction;

	stepper_event_callback_t event_callback;

	void *event_callback_user_data;
};

struct stepper_common_config_base {

	const struct stepper_common_config *const config;
};

struct stepper_common_data_base {

	struct stepper_common_data *const data;
};

int stepper_common_enable(const struct device *dev);

int stepper_common_disable(const struct device *dev);

int stepper_common_set_position(const struct device *dev, int32_t position);

int stepper_common_get_position(const struct device *dev, int32_t *position);

int stepper_common_move_to(const struct device *dev, int32_t position);

int stepper_common_move_by(const struct device *dev, int32_t micro_steps);

int stepper_common_step(const struct device *dev);

int stepper_common_set_direction(const struct device *dev, enum stepper_direction direction);

int stepper_common_is_moving(const struct device *dev, bool *is_moving);

int stepper_common_set_event_callback(const struct device *dev, stepper_event_callback_t callback,
				      void *user_data);

int stepper_common_set_position(const struct device *dev, int32_t position);

int stepper_common_get_position(const struct device *dev, int32_t *position);

int stepper_common_run(const struct device *dev, enum stepper_direction direction);

int stepper_common_stop(const struct device *dev);

void stepper_common_handle_event(const struct device *dev, enum stepper_event event);

#endif /* STEPPER_COMMON_H */
