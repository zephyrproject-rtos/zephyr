/*
 * Copyright 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_GPIO_STEPPER_COMMON_H_
#define ZEPHYR_DRIVER_STEPPER_GPIO_STEPPER_COMMON_H_

/**
 * @brief Stepper Driver APIs
 * @defgroup gpio_stepper Stepper Driver APIs
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/drivers/counter.h>
#include <stepper_ctrl_event_handler.h>

#include "stepper_timing_source.h"

/**
 * @brief Common GPIO stepper config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct gpio_stepper_common_config {
	const struct stepper_timing_source_api *timing_source;
	const struct device *counter;
	bool invert_direction;
	void (*timing_source_cb)(const struct device *dev);
};

/**
 * @brief Initialize common GPIO stepper config from devicetree instance.
 *        If the counter property is set, the timing source will be set to the counter timing
 *        source.
 *
 * @param node_id The devicetree node identifier.
 */
#define GPIO_STEPPER_DT_COMMON_CONFIG_INIT(node_id)                                                \
	{                                                                                          \
		.counter = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(node_id, counter)),                    \
		.invert_direction = DT_PROP(node_id, invert_direction),                            \
		.timing_source = COND_CODE_1(DT_NODE_HAS_PROP(node_id, counter),                   \
						(&step_counter_timing_source_api),                 \
						(&step_work_timing_source_api)),                   \
	}

/**
 * @brief Initialize common GPIO stepper config from devicetree instance.
 * @param inst Instance.
 */
#define GPIO_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst)                                              \
	GPIO_STEPPER_DT_COMMON_CONFIG_INIT(DT_DRV_INST(inst))

/**
 * @brief Common GPIO stepper data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct gpio_stepper_common_data {
	struct stepper_ctrl_event_handler_data event_common;
	struct k_spinlock lock;
	enum stepper_ctrl_direction direction;
	enum stepper_ctrl_run_mode run_mode;
	uint64_t microstep_interval_ns;
	uint64_t timing_source_interval_ns;
	atomic_t actual_position;
	atomic_t step_count;

	struct k_work_delayable stepper_dwork;
#ifdef CONFIG_GPIO_STEPPER_COUNTER_TIMING
	struct counter_top_cfg counter_top_cfg;
	bool counter_running;
#endif /* CONFIG_GPIO_STEPPER_COUNTER_TIMING */
};

/**
 * @brief Initialize common GPIO stepper data from devicetree instance.
 *
 * @param node_id The devicetree node identifier.
 */
#define GPIO_STEPPER_DT_COMMON_DATA_INIT(node_id)                                                  \
	{                                                                                          \
		.event_common = STEPPER_CTRL_EVENT_HANDLER_DT_DATA_INIT(node_id),                  \
	}

STEPPER_CONTROLLER_EVENT_COMMON_STRUCT_CHECK(struct gpio_stepper_common_data);

/**
 * @brief Initialize common GPIO stepper data from devicetree instance.
 * @param inst Instance.
 */
#define GPIO_STEPPER_DT_INST_COMMON_DATA_INIT(inst)                                                \
	GPIO_STEPPER_DT_COMMON_DATA_INIT(DT_DRV_INST(inst))

/**
 * @brief Validate the offset of the common data structures.
 *
 * @param config Name of the config structure.
 * @param data Name of the data structure.
 */
#define GPIO_STEPPER_STRUCT_CHECK(config, data)                                                    \
	BUILD_ASSERT(offsetof(config, common) == 0,                                                \
		     "struct gpio_stepper_common_config must be placed first");                    \
	BUILD_ASSERT(offsetof(data, common) == 0,                                                  \
		     "struct gpio_stepper_common_data must be placed first");

/**
 * @brief Common function to initialize a GPIO stepper device at init time.
 *
 * This function must be called at the end of the device init function.
 *
 * @param dev GPIO stepper device instance.
 *
 * @retval 0 If initialized successfully.
 * @retval -errno Negative errno in case of failure.
 */
int gpio_stepper_common_init(const struct device *dev);

/**
 * @brief Process the callback for a stepper_ctrl_event, ensuring the callback will be executed.
 *
 * @param dev Pointer to the device structure.
 * @param event The event for which to process the callback.
 */
static inline void gpio_stepper_common_process_cb(const struct device *dev,
						  const enum stepper_ctrl_event event)
{
	stepper_ctrl_event_handler_process_cb(dev, event);
}

/**
 * @brief Set the callback function to be called when a stepper motion controller event occurs
 *
 * @param dev Pointer to the device structure.
 * @param callback Callback function to be called when an event occurs.
 * @param user_data User data to be passed to the callback function.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline int gpio_stepper_common_set_event_cb(const struct device *dev,
						   stepper_ctrl_event_callback_t callback,
						   void *user_data)
{
	return stepper_ctrl_event_handler_set_event_cb(dev, callback, user_data);
}

/**
 * @brief Set the reference position of the stepper.
 *
 * @param dev Pointer to the device structure.
 * @param value The reference position value to set.
 * @return 0 on success, or a negative error code on failure.
 */
static inline int gpio_stepper_common_set_reference_position(const struct device *dev,
							     const int32_t value)
{
	struct gpio_stepper_common_data *data = dev->data;

	atomic_set(&data->actual_position, value);

	return 0;
}

/**
 * @brief Get the actual (reference) position of the stepper.
 *
 * @param dev Pointer to the device structure.
 * @param value Pointer to a variable where the position value will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
static inline int gpio_stepper_common_get_actual_position(const struct device *dev, int32_t *value)
{
	struct gpio_stepper_common_data *data = dev->data;

	*value = atomic_get(&data->actual_position);

	return 0;
}

/**
 * @brief Set the absolute target position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value The target position to set.
 * @return 0 on success, or a negative error code on failure.
 */
static inline int gpio_stepper_common_move_to(const struct device *dev, const int32_t value)
{
	struct gpio_stepper_common_data *data = dev->data;
	int32_t steps_to_move;

	/* Calculate the relative movement required */
	steps_to_move = value - atomic_get(&data->actual_position);

	return stepper_ctrl_move_by(dev, steps_to_move);
}

/**
 * @brief Check if the stepper motor is still moving.
 *
 * @param dev Pointer to the device structure.
 * @param is_moving Pointer to a boolean where the movement status will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
static inline int gpio_stepper_common_is_moving(const struct device *dev, bool *is_moving)
{
	const struct gpio_stepper_common_config *config = dev->config;

	*is_moving = config->timing_source->is_running(dev);

	return 0;
}

/**
 * @brief Update the direction of the stepper motor based on the step count.
 * @param dev Pointer to the device structure.
 */
static inline void gpio_stepper_common_update_direction_from_step_count(const struct device *dev)
{
	struct gpio_stepper_common_data *data = dev->data;

	if (atomic_get(&data->step_count) > 0) {
		data->direction = STEPPER_CTRL_DIRECTION_POSITIVE;
	} else if (atomic_get(&data->step_count) < 0) {
		data->direction = STEPPER_CTRL_DIRECTION_NEGATIVE;
	} else {
		/* Step count is zero, direction remains unchanged */
	}
}

/**
 * @brief Update the remaining steps to move for the stepper motor.
 * @param dev Pointer to the device structure.
 */
static inline void gpio_stepper_common_update_remaining_steps(const struct device *dev)
{
	struct gpio_stepper_common_data *data = dev->data;

	if (atomic_get(&data->step_count) > 0) {
		atomic_dec(&data->step_count);
	} else if (atomic_get(&data->step_count) < 0) {
		atomic_inc(&data->step_count);
	}
}

/**
 * @brief Task for controlling the stepper motor in position mode.
 * @param dev Pointer to the device structure.
 */
static inline void gpio_stepper_common_position_mode_task(const struct device *dev)
{
	struct gpio_stepper_common_data *data = dev->data;
	const struct gpio_stepper_common_config *config = dev->config;

	if (config->timing_source->needs_reschedule(dev) && atomic_get(&data->step_count) != 0) {
		(void)config->timing_source->start(dev);
	} else if (atomic_get(&data->step_count) == 0) {
		config->timing_source->stop(dev);
		gpio_stepper_common_process_cb(dev, STEPPER_CTRL_EVENT_STEPS_COMPLETED);
	}
}

/**
 * @brief Task for controlling the stepper motor in velocity mode.
 * @param dev Pointer to the device structure.
 */
static inline void gpio_stepper_common_velocity_mode_task(const struct device *dev)
{
	const struct gpio_stepper_common_config *config = dev->config;

	if (config->timing_source->needs_reschedule(dev)) {
		(void)config->timing_source->start(dev);
	}
}

/**
 * @brief Returns the device struct based on its gpio_stepper_common_data struct.
 * @param data Pointer to the gpio_stepper_common_data structure of the device.
 * @return device pointer
 */
static inline const struct device *
gpio_stepper_common_get_stepper_ctrl_dev(const struct gpio_stepper_common_data *data)
{
	ARG_UNUSED(data != NULL);
	return data->event_common.dev;
}
/** @} */

#endif /* ZEPHYR_DRIVER_STEPPER_GPIO_STEPPER_COMMON_H_ */
