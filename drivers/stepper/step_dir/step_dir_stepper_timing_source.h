/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_TIMING_SOURCE_H_
#define ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_TIMING_SOURCE_H_

#include "zephyr/drivers/counter.h"
#include "zephyr/kernel.h"
#include <zephyr/device.h>

/**
 * @brief Step-Dir function to call when time for one step has passed
 */
typedef void (*step_dir_step_handler)(const struct device *dev);

/**
 * @brief Initialize the stepper timing source.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_init)(const struct device *dev);

/**
 * @brief Update the stepper timing source.
 *
 * @param dev Pointer to the device structure.
 * @param update_data Pointer to struct containing the data needed to update the timing source.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_update)(const struct device *dev, void *update_data);

/**
 * @brief Start the stepper timing source.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_start)(const struct device *dev);

/**
 * @brief Stop the stepper timing source.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_stop)(const struct device *dev);

/**
 * @brief Check if the stepper timing source is running.
 *
 * @param dev Pointer to the device structure.
 * @return true if the timing source is running, false otherwise.
 */
typedef bool (*stepper_timing_source_is_running)(const struct device *dev);

/**
 * @brief Returns the current timing interval of the timing source.
 *
 * @param dev Pointer to the device structure.
 * @return Current timing interval if the timing source is running, 0 otherwise.
 */
typedef uint64_t (*stepper_timing_source_get_current_interval)(const struct device *dev);

/**
 * @brief Registers step-dir function to call when time for one step has passed.
 *
 * @param dev Pointer to the device structure.
 * @param handler Step dir function to call.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_register_step_handler)(const struct device *dev,
							   step_dir_step_handler handler);

/**
 * @brief Stepper timing source API.
 */
struct stepper_timing_source_api {
	stepper_timing_source_init init;
	stepper_timing_source_update update;
	stepper_timing_source_start start;
	stepper_timing_source_stop stop;
	stepper_timing_source_is_running is_running;
	stepper_timing_source_get_current_interval get_current_interval;
	stepper_timing_source_register_step_handler register_step_handler;
};

/**
 * @brief Data struct for the work-queue timing source
 */
struct step_work_data {
	const struct device *dev;
	uint64_t microstep_interval_ns;
	struct k_work_delayable stepper_dwork;
	step_dir_step_handler handler;
};

#define STEP_DIR_TIMING_SOURCE_WORK_DATA_INIT(node_id)                                             \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	}

extern const struct stepper_timing_source_api step_work_timing_source_api;

#ifdef CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING
/**
 * @brief Data struct for the counter timing source
 */
struct step_counter_data {
	const struct device *dev;
	const struct device *counter;
	struct counter_top_cfg counter_top_cfg;
	bool counter_running;
	step_dir_step_handler handler;
};

/**
 * @brief Data struct for the counter-acceleration timing source
 */
struct step_counter_accel_data {
	const struct device *dev;
	const struct device *counter;
	struct counter_top_cfg counter_top_cfg;
	step_dir_step_handler handler;
	bool counter_running;
	uint64_t root_factor;
	uint32_t accurate_steps;
	uint32_t pulse_index;
	uint64_t current_time_int;
	uint64_t base_time_int;
	uint64_t frequency;
	uint64_t current_interval;
	bool dual_edge;
	bool flip_state;
};

/**
 * @brief Initialize counter timing source data from devicetree instance.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_TIMING_SOURCE_COUNTER_DATA_INIT(node_id)                                          \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.counter = DEVICE_DT_GET(DT_PHANDLE(node_id, counter)),                            \
	}

/**
 * @brief Initialize counter-acceleration timing source data from devicetree instance.
 *
 * @param node_id The devicetree node identifier.
 */
#define STEP_DIR_TIMING_SOURCE_COUNTER_ACCEL_DATA_INIT(node_id)                                    \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.counter = DEVICE_DT_GET(DT_PHANDLE(node_id, counter)),                            \
		.root_factor = 1000000,                                                            \
		.accurate_steps = DT_PROP_OR(node_id, accurate_steps, 15),                         \
		.dual_edge = DT_PROP_OR(node_id, dual_edge_step, false),                           \
	}

extern const struct stepper_timing_source_api step_counter_timing_source_api;
extern const struct stepper_timing_source_api step_counter_accel_timing_source_api;
#endif /* CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING */

/**
 * @brief Union containing the data struct of the timing source
 */
union step_dir_timing_source_data {
	struct step_work_data work;
#ifdef CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING
	struct step_counter_data counter;
	struct step_counter_accel_data counter_accel;
#endif /* CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING */
};

#define STEP_DIR_TIMING_SOURCE_SELECT(node_id)                                                     \
	(COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, timing_source, counter),			   \
			(&step_counter_timing_source_api),					   \
	(COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, timing_source, work),				   \
			(&step_work_timing_source_api),						   \
	(COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, timing_source, counter_accel),			   \
			(&step_counter_accel_timing_source_api), ()))))))

#define STEP_DIR_TIMING_SOURCE_DATA(node_id)                                                       \
	COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, timing_source, counter),				   \
			(.ts_data.counter = STEP_DIR_TIMING_SOURCE_COUNTER_DATA_INIT(node_id),),   \
	(COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, timing_source, work),				   \
			(.ts_data.work = STEP_DIR_TIMING_SOURCE_WORK_DATA_INIT(node_id),),	   \
	(COND_CODE_1(DT_ENUM_HAS_VALUE(node_id, timing_source, counter_accel),			   \
			(.ts_data.counter_accel =						   \
			STEP_DIR_TIMING_SOURCE_COUNTER_ACCEL_DATA_INIT(node_id),), ())))))

#define STEP_DIR_TIMING_SOURCE_STRUCT_CHECK(data)                                                  \
	BUILD_ASSERT(offsetof(data, ts_data) == 0,                                                 \
		     "timing source data must be placed first must be placed first");

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_TIMING_SOURCE_H_ */
