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
#include <zephyr/drivers/counter.h>

#include "step_dir_stepper_timing_source.h"

/**
 * @brief Common step direction stepper config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct step_dir_stepper_common_config {
	const struct gpio_dt_spec step_pin;
	const struct gpio_dt_spec dir_pin;
	bool dual_edge;
	const struct stepper_timing_source_api *timing_source;
	const struct device *counter;
	bool invert_direction;
};

/**
 * @brief Initialize common step direction stepper config from devicetree instance.
 *        If the counter property is set, the timing source will be set to the counter timing
 *        source.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_STEPPER_DT_COMMON_CONFIG_INIT(node_id)                                            \
	{                                                                                          \
		.step_pin = GPIO_DT_SPEC_GET(node_id, step_gpios),                                 \
		.dir_pin = GPIO_DT_SPEC_GET(node_id, dir_gpios),                                   \
		.dual_edge = DT_PROP_OR(node_id, dual_edge_step, false),                           \
		.counter = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(node_id, counter)),                    \
		.invert_direction = DT_PROP(node_id, invert_direction),                            \
		.timing_source = COND_CODE_1(DT_NODE_HAS_PROP(node_id, counter),                   \
						(&step_counter_timing_source_api),                 \
						(&step_work_timing_source_api)),    \
		}

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
	int32_t actual_position;
	uint64_t microstep_interval_ns;
	int32_t step_count;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
	step_dir_step_handler handler;

	struct k_work_delayable stepper_dwork;

#ifdef CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING
	struct counter_top_cfg counter_top_cfg;
	bool counter_running;
#endif /* CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING */

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
#define STEP_DIR_STEPPER_DT_COMMON_DATA_INIT(node_id)                                              \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	}

/**
 * @brief Setup macro for step_dir_stepper_common
 *
 * While the macro is empty for this step dir implementation, other step dir implementations can use
 * it for things like pinctrl.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_STEPPER_DT_COMMON_SETUP(node_id)

/** @} */

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_COMMON_H_ */
