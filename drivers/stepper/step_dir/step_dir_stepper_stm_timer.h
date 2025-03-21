/*
 * Copyright 2025 Copyright (c) 2024 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_STM_TIMER_H_
#define ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_STM_TIMER_H_

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
#include <stm32_ll_rcc.h>
#include <stm32_ll_tim.h>
#include <zephyr/drivers/pinctrl.h>

/**
 * @brief Common step direction stepper config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct step_dir_stepper_stm_timer_config {
	const struct gpio_dt_spec dir_pin;
	const struct device *step_generator;
	const struct device *step_counter;
	uint32_t trigger_input;
	const struct pinctrl_dev_config *pcfg;
	TIM_TypeDef *tim_gen;
	TIM_TypeDef *tim_count;
	uint32_t output_channel;
	uint32_t initial_step_gen_prescalar;
};

/**
 * @brief Initialize common step direction stepper config from devicetree instance.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_STEPPER_DT_STM_TIMER_CONFIG_INIT(node_id)                                         \
	{                                                                                          \
		.dir_pin = GPIO_DT_SPEC_GET(node_id, dir_gpios),                                   \
		.step_generator = DEVICE_DT_GET(DT_PHANDLE(node_id, step_generator)),              \
		.step_counter = DEVICE_DT_GET(DT_PHANDLE(node_id, step_counter)),                  \
		.trigger_input = DT_PROP(node_id, trigger_input),                                  \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(node_id),                                        \
		.tim_gen = ((TIM_TypeDef *)DT_REG_ADDR(                                            \
			DT_PARENT(DT_PHANDLE(node_id, step_generator)))),                          \
		.tim_count = ((TIM_TypeDef *)DT_REG_ADDR(                                          \
			DT_PARENT(DT_PHANDLE(node_id, step_counter)))),                            \
		.output_channel = DT_PROP(node_id, output_channel),                                \
		.initial_step_gen_prescalar =                                                      \
			DT_PROP(DT_PARENT(DT_PHANDLE(node_id, step_generator)), st_prescaler),     \
	}

/**
 * @brief Initialize common step direction stepper config from devicetree instance.
 * @param inst Instance.
 */
#define STEP_DIR_STEPPER_DT_INST_STM_TIMER_CONFIG_INIT(inst)                                       \
	STEP_DIR_STEPPER_DT_STM_TIMER_CONFIG_INIT(DT_DRV_INST(inst))

/**
 * @brief Common step direction stepper data.
 *
 * This structure **must** be placed first in the driver's data structure.
 */
struct step_dir_stepper_stm_timer_data {
	const struct device *dev;
	struct k_spinlock lock;
	enum stepper_direction direction;
	enum stepper_run_mode run_mode;
	atomic_t actual_position;
	uint64_t microstep_interval_ns;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
	struct k_work_delayable stepper_dwork;
	bool counter_running;
	struct counter_top_cfg cfg_gen;
	struct counter_top_cfg cfg_count;
	uint32_t counter_gen_base_freq;

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
#define STEP_DIR_STEPPER_DT_STM_TIMER_DATA_INIT(node_id)                                           \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	}

/**
 * @brief Initialize common step direction stepper data from devicetree instance.
 * @param inst Instance.
 */
#define STEP_DIR_STEPPER_DT_INST_STM_TIMER_DATA_INIT(inst)                                         \
	STEP_DIR_STEPPER_DT_STM_TIMER_DATA_INIT(DT_DRV_INST(inst))

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
 * @brief Execute step-dir specific setup/configuration outside config and data
 * @param inst Instance
 */
#define STEP_DIR_STEPPER_STM_TIMER_INST_SETUP(inst) PINCTRL_DT_INST_DEFINE(inst)

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
int step_dir_stepper_stm_timer_init(const struct device *dev);

/**
 * @brief Move the stepper motor by a given number of micro_steps.
 *
 * @param dev Pointer to the device structure.
 * @param micro_steps Number of micro_steps to move. Can be positive or negative.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_stm_timer_move_by(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Set the step interval of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param microstep_interval_ns The step interval in nanoseconds.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_stm_timer_set_microstep_interval(const struct device *dev,
						      const uint64_t microstep_interval_ns);

/**
 * @brief Set the reference position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value The reference position value to set.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_stm_timer_set_reference_position(const struct device *dev,
						      const int32_t value);

/**
 * @brief Get the actual (reference) position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value Pointer to a variable where the position value will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_stm_timer_get_actual_position(const struct device *dev, int32_t *value);

/**
 * @brief Set the absolute target position of the stepper motor.
 *
 * @param dev Pointer to the device structure.
 * @param value The target position to set.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_stm_timer_move_to(const struct device *dev, const int32_t value);

/**
 * @brief Check if the stepper motor is still moving.
 *
 * @param dev Pointer to the device structure.
 * @param is_moving Pointer to a boolean where the movement status will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_stm_timer_is_moving(const struct device *dev, bool *is_moving);

/**
 * @brief Run the stepper with a given direction and step interval.
 *
 * @param dev Pointer to the device structure.
 * @param direction The direction of movement (positive or negative).
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_stm_timer_run(const struct device *dev,
				   const enum stepper_direction direction);

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
int step_dir_stepper_stm_timer_set_event_callback(const struct device *dev,
						  stepper_event_callback_t callback,
						  void *user_data);

/**
 * @brief Stops the step-dir implementation
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
int step_dir_stepper_stm_timer_stop(const struct device *dev);

/** @} */

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_STM_TIMER_H_ */
