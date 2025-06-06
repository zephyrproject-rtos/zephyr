/*
 * Copyright 2025 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_H_
#define ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_H_

/**
 * @brief Stepper Driver APIs
 * @defgroup step_dir_stepper Stepper Driver APIs
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/counter.h>

#include "step_dir_stepper_common.h"

/**
 * @brief Function to initialize a step direction stepper device at init time.
 *
 * This function must be called at the end of the device init function.
 *
 * @param dev Step direction stepper device instance.
 *
 * @retval 0 If initialized successfully.
 * @retval -errno Negative errno in case of failure.
 */
typedef int (*step_dir_stepper_init_t)(const struct device *dev);

/**
 * @brief Move the stepper motor by a given number of micro_steps.
 *
 * @param dev Pointer to the device structure.
 * @param micro_steps Number of micro_steps to move. Can be positive or negative.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*step_dir_stepper_move_by_t)(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Move the stepper motor to the target position.
 *
 * @param dev Pointer to the device structure.
 * @param value The target position to set.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*step_dir_stepper_move_to_t)(const struct device *dev, const int32_t value);

/**
 * @brief Set the step interval of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param microstep_interval_ns The step interval in nanoseconds.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*step_dir_stepper_set_microstep_interval_t)(const struct device *dev,
							 const uint64_t microstep_interval_ns);

/**
 * @brief Set the reference position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value The reference position value to set.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*step_dir_stepper_set_reference_position_t)(const struct device *dev,
							 const int32_t value);

/**
 * @brief Get the actual (reference) position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value Pointer to a variable where the position value will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*step_dir_stepper_get_actual_position_t)(const struct device *dev, int32_t *value);

/**
 * @brief Check if the stepper motor is still moving.
 *
 * @param dev Pointer to the device structure.
 * @param is_moving Pointer to a boolean where the movement status will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*step_dir_stepper_is_moving_t)(const struct device *dev, bool *is_moving);

/**
 * @brief Run the stepper with a given direction and step interval.
 *
 * @param dev Pointer to the device structure.
 * @param direction The direction of movement (positive or negative).
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*step_dir_stepper_run_t)(const struct device *dev,
				      const enum stepper_direction direction);

/**
 * @brief Stop the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*step_dir_stepper_stop_t)(const struct device *dev);

/**
 * @brief Set a callback function for stepper motor events.
 *
 * This function sets a user-defined callback that will be invoked when a stepper motor event
 * occurs.
 *
 * @param dev Pointer to the device structure.
 * @param callback The callback function to set.
 * @param user_data Pointer to user-defined data that will be passed to the callback.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*step_dir_stepper_set_event_callback_t)(const struct device *dev,
						     stepper_event_callback_t callback,
						     void *user_data);

/**
 * @brief Handle a timing signal and update the stepper position.
 * @param dev Pointer to the device structure.
 */
typedef void (*step_dir_stepper_handle_timing_signal_t)(const struct device *dev);

/**
 * @brief Trigger callback function for stepper motor events.
 * @param dev Pointer to the device structure.
 * @param event The stepper_event to rigger the callback for.
 */
typedef void (*step_dir_stepper_trigger_callback_t)(const struct device *dev,
						    enum stepper_event event);

/**
 * @brief Step-Dir API.
 */
struct step_dir_stepper_api {
	step_dir_stepper_init_t init;
	step_dir_stepper_move_by_t move_by;
	step_dir_stepper_move_to_t move_to;
	step_dir_stepper_set_microstep_interval_t set_microstep_interval;
	step_dir_stepper_set_reference_position_t set_reference_position;
	step_dir_stepper_get_actual_position_t get_actual_position;
	step_dir_stepper_is_moving_t is_moving;
	step_dir_stepper_run_t run;
	step_dir_stepper_stop_t stop;
	step_dir_stepper_set_event_callback_t set_event_callback;
	step_dir_stepper_handle_timing_signal_t handle_timing_signal;
	step_dir_stepper_trigger_callback_t trigger_callback;
};

/**
 * @brief Union containing the config struct of the step_dir implementations
 */
union step_dir_stepper_config {
	struct step_dir_stepper_common_config common;
};

/**
 * @brief Union containing the data struct of the step_dir implementations
 */
union step_dir_stepper_data {
	struct step_dir_stepper_common_data common;
};

/**
 * @brief Validate the offset of the data structures.
 *
 * @param config Name of the config structure.
 * @param data Name of the data structure.
 */
#define STEP_DIR_STEPPER_STRUCT_CHECK(config, data)                                                \
	BUILD_ASSERT(offsetof(config, step_dir_config) == 0,                                       \
		     "struct step_dir_stepper_config must be placed first");                       \
	BUILD_ASSERT(offsetof(data, step_dir_data) == 0,                                           \
		     "struct step_dir_stepper_data must be placed first");

extern const struct step_dir_stepper_api step_dir_stepper_common_api;

#define STEP_DIR_STEPPER_SETUP(node_id)                                                            \
	COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, step_dir, common),				   \
			(STEP_DIR_STEPPER_DT_COMMON_SETUP(node_id)), ())

#define STEP_DIR_STEPPER_SELECT(node_id)                                                           \
	COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, step_dir, common), (&step_dir_stepper_common_api),  \
	())

#define STEP_DIR_STEPPER_CONFIG(node_id)                                                           \
	COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, step_dir, common),				   \
			(.step_dir_config.common =                                                 \
				STEP_DIR_STEPPER_DT_COMMON_CONFIG_INIT(node_id)), ())

#define STEP_DIR_STEPPER_DATA(node_id)                                                             \
	COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, step_dir, common),				   \
			(.step_dir_data.common = STEP_DIR_STEPPER_DT_COMMON_DATA_INIT(node_id)), ())

#define STEP_DIR_STEPPER_INST_SETUP(inst)  STEP_DIR_STEPPER_SETUP(DT_DRV_INST(inst))
#define STEP_DIR_STEPPER_SELECT_INST(inst) STEP_DIR_STEPPER_SELECT(DT_DRV_INST(inst))
#define STEP_DIR_STEPPER_CONFIG_INST(inst) STEP_DIR_STEPPER_CONFIG(DT_DRV_INST(inst))
#define STEP_DIR_STEPPER_DATA_INST(inst)   STEP_DIR_STEPPER_DATA(DT_DRV_INST(inst))

/**
 * @brief Wrapper function for assigning `step_dir_stepper_move_by` to the stepper api.
 *
 * @param dev Pointer to the device structure.
 * @param micro_steps Number of micro_steps to move. Can be positive or negative.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_move_by(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Wrapper function for assigning `step_dir_stepper_set_microstep_interval` to the stepper
 * api.
 *
 * @param dev Pointer to the device structure.
 * @param microstep_interval_ns The step interval in nanoseconds.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_set_microstep_interval(const struct device *dev,
					    const uint64_t microstep_interval_ns);

/**
 * @brief Wrapper function for assigning `step_dir_stepper_set_reference_position` to the stepper
 * api.
 *
 * @param dev Pointer to the device structure.
 * @param value The reference position value to set.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_set_reference_position(const struct device *dev, const int32_t value);

/**
 * @brief Wrapper function for assigning `step_dir_stepper_get_actual_position` to the stepper api.
 *
 * @param dev Pointer to the device structure.
 * @param value Pointer to a variable where the position value will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_get_actual_position(const struct device *dev, int32_t *value);

/**
 * @brief Wrapper function for assigning `step_dir_stepper_move_to` to the stepper api.
 *
 * @param dev Pointer to the device structure.
 * @param value The target position to set.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_move_to(const struct device *dev, const int32_t value);

/**
 * @brief Wrapper function for assigning `step_dir_stepper_is_moving` to the stepper api.
 *
 * @param dev Pointer to the device structure.
 * @param is_moving Pointer to a boolean where the movement status will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_is_moving(const struct device *dev, bool *is_moving);

/**
 * @brief Wrapper function for assigning `step_dir_stepper_run` to the stepper api.
 *
 * @param dev Pointer to the device structure.
 * @param direction The direction of movement (positive or negative).
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_run(const struct device *dev, const enum stepper_direction direction);

/**
 * @brief Wrapper function for assigning `step_dir_stepper_stop` to the stepper api.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_stop(const struct device *dev);

/**
 * @brief Wrapper function for assigning `step_dir_stepper_set_event_callback` to the stepper api.
 *
 * @param dev Pointer to the device structure.
 * @param callback The callback function to set.
 * @param user_data Pointer to user-defined data that will be passed to the callback.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_set_event_callback(const struct device *dev, stepper_event_callback_t callback,
					void *user_data);

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_H_ */
