/*
 * Copyright 2025 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_
#define ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_

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

#include "step_dir_stepper_timing_source.h"

/**
 * @brief Common step direction stepper config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct step_dir_stepper_common_accel_config {
	const struct gpio_dt_spec step_pin;
	const struct gpio_dt_spec dir_pin;
	bool dual_edge;
	const struct stepper_timing_source_api *timing_source;
};

/**
 * @brief Initialize common step direction stepper config from devicetree instance.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_STEPPER_DT_COMMON_ACCEL_CONFIG_INIT(node_id)                                      \
	{                                                                                          \
		.step_pin = GPIO_DT_SPEC_GET(node_id, step_gpios),                                 \
		.dir_pin = GPIO_DT_SPEC_GET(node_id, dir_gpios),                                   \
		.dual_edge = DT_PROP_OR(node_id, dual_edge_step, false),                           \
		.timing_source = STEP_DIR_TIMING_SOURCE_SELECT(node_id),                           \
	}

/**
 * @brief Initialize common step direction stepper config from devicetree instance.
 * @param inst Instance.
 */
#define STEP_DIR_STEPPER_DT_INST_COMMON_ACCEL_CONFIG_INIT(inst)                                    \
	STEP_DIR_STEPPER_DT_COMMON_ACCEL_CONFIG_INIT(DT_DRV_INST(inst))

/**
 * @brief Struct used to update the timing source.
 */
struct step_dir_accel_timing_data {
	uint64_t microstep_interval_ns;
	uint64_t start_microstep_interval;
	uint32_t acceleration;
};

/**
 * @brief Common step direction stepper data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct step_dir_stepper_common_accel_data {
	union step_dir_timing_source_data ts_data;
	const struct device *dev;
	struct k_spinlock lock;
	enum stepper_direction direction;
	enum stepper_run_mode run_mode;
	int32_t actual_position;
	uint64_t microstep_interval_ns;
	int32_t step_count;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
	uint32_t acceleration;
	uint64_t current_interval;
	struct step_dir_accel_timing_data timing_data;
	uint32_t const_steps;
	uint32_t decel_steps;
	uint32_t accel_steps;
	uint32_t step_index;
	const struct gpio_dt_spec *step_pin;
	uint64_t new_interval;
	bool stopping;
	bool step_pin_low;

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS
	struct k_work event_callback_work;
	struct k_msgq event_msgq;
	uint8_t event_msgq_buffer[CONFIG_STEPPER_STEP_DIR_EVENT_QUEUE_LEN *
				  sizeof(enum stepper_event)];
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */
};

/**
 * @brief Initialize common step direction stepper data from devicetree instance.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_STEPPER_DT_COMMON_ACCEL_DATA_INIT(node_id)                                        \
	{.dev = DEVICE_DT_GET(node_id),                                                            \
	 .acceleration = DT_PROP_OR(node_id, acceleration, 0),                                     \
	 STEP_DIR_TIMING_SOURCE_DATA(node_id)}

/**
 * @brief Initialize common step direction stepper data from devicetree instance.
 * @param inst Instance.
 */
#define STEP_DIR_STEPPER_DT_INST_COMMON_ACCEL_DATA_INIT(inst)                                      \
	STEP_DIR_STEPPER_DT_COMMON_ACCEL_DATA_INIT(DT_DRV_INST(inst))

/**
 * @brief Validate the offset of the common data structures.
 *
 * @param config Name of the config structure.
 * @param data Name of the data structure.
 */
#define STEP_DIR_STEPPER_ACCEL_STRUCT_CHECK(config, data)                                          \
	BUILD_ASSERT(offsetof(config, common) == 0,                                                \
		     "struct step_dir_stepper_common_accel_config must be placed first");          \
	BUILD_ASSERT(offsetof(data, common) == 0,                                                  \
		     "struct step_dir_stepper_common_accel_data must be placed first");

/**
 * @brief Common function to initialize a step direction stepper device at init time.
 *
 * This function must be called at the end of the device init function.
 *
 * @param dev Step direction stepper device instance.
 *
 * @retval 0 If initialized successfully.
 * @retval -errno Negative errno in case of failure.
 */
int step_dir_stepper_common_accel_init(const struct device *dev);

/**
 * @brief Move the stepper motor by a given number of micro_steps.
 *
 * @param dev Pointer to the device structure.
 * @param micro_steps Number of micro_steps to move. Can be positive or negative.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_move_by(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Set the step interval of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param microstep_interval_ns The step interval in nanoseconds.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_set_microstep_interval(const struct device *dev,
							 const uint64_t microstep_interval_ns);

/**
 * @brief Set the reference position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value The reference position value to set.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_set_reference_position(const struct device *dev,
							 const int32_t value);

/**
 * @brief Get the actual (reference) position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value Pointer to a variable where the position value will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_get_actual_position(const struct device *dev, int32_t *value);

/**
 * @brief Set the absolute target position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value The target position to set.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_move_to(const struct device *dev, const int32_t value);

/**
 * @brief Check if the stepper motor is still moving.
 *
 * @param dev Pointer to the device structure.
 * @param is_moving Pointer to a boolean where the movement status will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_is_moving(const struct device *dev, bool *is_moving);

/**
 * @brief Run the stepper with a given direction and step interval.
 *
 * @param dev Pointer to the device structure.
 * @param direction The direction of movement (positive or negative).
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_run(const struct device *dev,
				      const enum stepper_direction direction);

/**
 * @brief Stop the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_stop(const struct device *dev);

/**
 * @brief Immediatly stop the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_stop_immediate(const struct device *dev);

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
int step_dir_stepper_common_accel_set_event_callback(const struct device *dev,
						     stepper_event_callback_t callback,
						     void *user_data);

/**
 * @brief Updates driver accelarion. Takes effect on next movement command.
 *
 * This function updates the acceleration for ths driver. The new acceleration value is used from
 * the next movement command onwars, any currently running movements are unaffected. An acceleration
 * value of 0 causes the target velocity to be immediatly reached during movement, skipping the
 * acceleration ramp.
 *
 * @param dev Pointer to the device structure.
 * @param acceleration The new acceleration.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_accel_update_acceleraion(const struct device *dev,
						     uint32_t acceleration);

/**
 * @brief Handle a timing signal and update the stepper position.
 * @param dev Pointer to the device structure.
 */
void stepper_handle_timing_signal_accel(const struct device *dev);

/** @} */

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_ */
