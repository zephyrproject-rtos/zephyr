/*
 * Copyright 2024 Fabian Blatz <fabianblatz@gmail.com>
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

/**
 * @brief Common step direction stepper config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct step_dir_stepper_common_config {
	const struct gpio_dt_spec step_pin;
	const struct gpio_dt_spec dir_pin;
	bool dual_edge;
};

/**
 * @brief Initialize common step direction stepper config from devicetree instance.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_STEPPER_DT_COMMON_CONFIG_INIT(node_id)                                            \
	{                                                                                          \
		.step_pin = GPIO_DT_SPEC_GET(node_id, step_gpios),                                 \
		.dir_pin = GPIO_DT_SPEC_GET(node_id, dir_gpios),	                           \
		.dual_edge = DT_PROP_OR(node_id, dual_edge_step, false),                           \
	}

/**
 * @brief Initialize common step direction stepper config from devicetree instance.
 * @param inst Instance.
 */
#define STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst)                                          \
	STEP_DIR_STEPPER_DT_COMMON_CONFIG_INIT(DT_DRV_INST(inst))

/**
 * @brief Common step direction stepper data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct step_dir_stepper_common_data {
	const struct device *dev;
	struct k_spinlock lock;
	enum stepper_direction direction;
	enum stepper_run_mode run_mode;
	struct k_work_delayable stepper_dwork;
	int32_t actual_position;
	uint32_t delay_in_us;
	int32_t step_count;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
};

/**
 * @brief Initialize common step direction stepper data from devicetree instance.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_STEPPER_DT_COMMON_DATA_INIT(node_id)                                              \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	}

/**
 * @brief Initialize common step direction stepper data from devicetree instance.
 * @param inst Instance.
 */
#define STEP_DIR_STEPPER_DT_INST_COMMON_DATA_INIT(inst)                                            \
	STEP_DIR_STEPPER_DT_COMMON_DATA_INIT(DT_DRV_INST(inst))

/**
 * @brief Validate the offset of the common data structures.
 *
 * @param config Name of the config structure.
 * @param data Name of the data structure.
 */
#define STEP_DIR_STEPPER_STRUCT_CHECK(config, data)                                                \
	BUILD_ASSERT(offsetof(config, common) == 0,                                                \
		     "struct step_dir_stepper_common_config must be placed first");                \
	BUILD_ASSERT(offsetof(data, common) == 0,                                                  \
		     "struct step_dir_stepper_common_data must be placed first");

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
int step_dir_stepper_common_init(const struct device *dev);

/**
 * @brief Move the stepper motor by a given number of micro_steps.
 *
 * @param dev Pointer to the device structure.
 * @param micro_steps Number of micro_steps to move. Can be positive or negative.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_move_by(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Set the maximum velocity in micro_steps per second.
 *
 * @param dev Pointer to the device structure.
 * @param velocity Maximum velocity in micro_steps per second.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_set_max_velocity(const struct device *dev, const uint32_t velocity);

/**
 * @brief Set the reference position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value The reference position value to set.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_set_reference_position(const struct device *dev, const int32_t value);

/**
 * @brief Get the actual (reference) position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value Pointer to a variable where the position value will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_get_actual_position(const struct device *dev, int32_t *value);

/**
 * @brief Set the absolute target position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value The target position to set.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_move_to(const struct device *dev, const int32_t value);

/**
 * @brief Check if the stepper motor is still moving.
 *
 * @param dev Pointer to the device structure.
 * @param is_moving Pointer to a boolean where the movement status will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_is_moving(const struct device *dev, bool *is_moving);

/**
 * @brief Run the stepper with a given velocity in a given direction.
 *
 * @param dev Pointer to the device structure.
 * @param direction The direction of movement (positive or negative).
 * @param velocity The velocity in micro_steps per second.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_common_run(const struct device *dev, const enum stepper_direction direction,
				const uint32_t velocity);

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
int step_dir_stepper_common_set_event_callback(const struct device *dev,
					       stepper_event_callback_t callback, void *user_data);

/** @} */

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_ */
