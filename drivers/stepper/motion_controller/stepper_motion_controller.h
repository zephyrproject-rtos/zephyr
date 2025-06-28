/*
 * Copyright 2024 Fabian Blatz <fabianblatz@gmail.com>
 * Copyright 2025 Andre Stefanov <mail@andrestefanov.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STEPPER_MOTION_CONTROLLER_H
#define STEPPER_MOTION_CONTROLLER_H

/**
 * @brief Stepper Driver APIs
 * @defgroup step_dir_stepper Stepper Driver APIs
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>

#include "timing_source/stepper_timing_source.h"
#include "ramp/stepper_ramp.h"

typedef void (*stepper_motion_controller_step_callback_t)(const struct device *);

typedef void (*stepper_motion_controller_set_direction_callback_t)(const struct device *,
								   enum stepper_direction);

typedef void (*stepper_motion_controller_event_callback_t)(const struct device *,
							   enum stepper_event);

struct stepper_motion_controller_callbacks_api {
	stepper_motion_controller_step_callback_t step;
	stepper_motion_controller_set_direction_callback_t set_direction;
	stepper_motion_controller_event_callback_t event;
};

/**
 * @brief Common step direction stepper config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct stepper_motion_controller_config {
	struct stepper_timing_source *timing_source;

	const struct stepper_motion_controller_callbacks_api *callbacks;
};

/**
 * @brief Common step direction stepper data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct stepper_motion_controller_data {
	struct k_spinlock lock;
	enum stepper_direction direction;
	int32_t target_position;

	// Position tracking
	int32_t position;

	struct stepper_ramp ramp;
};

#define STEPPER_MOTION_CONTROLLER_DT_INST_DEFINE(inst, callbacks_param)                            \
	STEPPER_TIMING_SOURCE_DT_INST_DEFINE(inst)                                                 \
	static struct stepper_motion_controller_data stepper_motion_controller_data_##inst = {};   \
	static const struct stepper_motion_controller_config                                       \
		stepper_motion_controller_cfg_##inst = {                                           \
			.timing_source = STEPPER_TIMING_SOURCE_DT_INST_GET(inst),                  \
			.callbacks = callbacks_param,                                              \
	};

#define STEPPER_MOTION_CONTROLLER_DT_INST_GET_CONFIG(inst) (&stepper_motion_controller_cfg_##inst)
#define STEPPER_MOTION_CONTROLLER_DT_INST_GET_DATA(inst) (&stepper_motion_controller_data_##inst)

/**
 * @brief Common function to initialize a step direction stepper device at init time.
 *
 * This function must be called at the end of the device init function.
 *
 * @param dev Pointer to the stepper device structure.
 *
 * @retval 0 If initialized successfully.
 * @retval -errno Negative errno in case of failure.
 */
int stepper_motion_controller_init(const struct device *dev);

/**
 * @brief Move the stepper motor by a given number of micro_steps.
 *
 * @param dev Pointer to the device structure.
 * @param micro_steps Number of micro_steps to move. Can be positive or negative.
 * @return 0 on success, or a negative error code on failure.
 */
int stepper_motion_controller_move_by(const struct device *dev, int32_t micro_steps);

/**
 * @brief Move the stepper motor to an absolute position.
 *
 * @param dev Pointer to the device structure.
 * @param position Absolute position to move to.
 * @return 0 on success, or a negative error code on failure.
 */
int stepper_motion_controller_move_to(const struct device *dev, int32_t position);

/**
 * @brief Set the current position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param position The position to set as current.
 * @return 0 on success, or a negative error code on failure.
 */
int stepper_motion_controller_set_position(const struct device *dev, int32_t position);

/**
 * @brief Get the current position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param position Pointer to store the current position.
 * @return 0 on success, or a negative error code on failure.
 */
int stepper_motion_controller_get_position(const struct device *dev, int32_t *position);

int stepper_motion_controller_set_ramp(const struct device *dev,
				       const struct stepper_ramp_profile *ramp);

/**
 * @brief Stop the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
int stepper_motion_controller_stop(const struct device *dev);

/**
 * @brief Run the stepper motor continuously in a given direction.
 *
 * @param dev Pointer to the device structure.
 * @param direction The direction to run continuously.
 * @return 0 on success, or a negative error code on failure.
 */
int stepper_motion_controller_run(const struct device *dev, enum stepper_direction direction);

/**
 * @brief Check if the stepper motor is currently moving.
 *
 * @param dev Pointer to the device structure.
 * @param is_moving Pointer to store the moving status.
 * @return 0 on success, or a negative error code on failure.
 */
int stepper_motion_controller_is_moving(const struct device *dev, bool *is_moving);

/** @} */

#endif /* STEPPER_MOTION_CONTROLLER_H */
