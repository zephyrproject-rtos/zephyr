/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *			   Copyright (c) 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_MOTION_CONTROLLER_TIMING_SOURCE_H_
#define ZEPHYR_DRIVER_STEPPER_MOTION_CONTROLLER_TIMING_SOURCE_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/counter.h>

typedef void (*stepper_timing_source_callback_t)(const void *user_data);

struct stepper_timing_source_counter_cfg {
	const struct device *dev;
};

#define STEPPER_TIMING_SOURCE_DT_INST_COUNTER_CFG_DEFINE(inst)                                     \
	static const struct stepper_timing_source_counter_cfg stepper_timing_source_cfg_##inst = { \
		.dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, counter)),                              \
	};

struct stepper_timing_source_counter_data {
	struct counter_top_cfg counter_top_cfg;
};

#define STEPPER_TIMING_SOURCE_DT_INST_COUNTER_DATA_DEFINE(inst)                                    \
	static struct stepper_timing_source_counter_data stepper_timing_source_data_##inst = {};

struct stepper_timing_source_work_data {
	struct k_work_delayable dwork;
	const struct stepper_timing_source *timing_source;
};

#define STEPPER_TIMING_SOURCE_DT_INST_WORK_DATA_DEFINE(inst)                                       \
	static struct stepper_timing_source_work_data stepper_timing_source_data_##inst = {        \
		.timing_source = STEPPER_TIMING_SOURCE_DT_INST_GET(inst)};

struct stepper_timing_source {
	const void *const config;
	void *const data;

	stepper_timing_source_callback_t callback;
	const void *user_data;
};

#define STEPPER_TIMING_DT_INST_USE_COUNTER(inst)                                                   \
	UTIL_AND(CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING, DT_INST_NODE_HAS_PROP(inst, counter))

#define STEPPER_TIMING_SOURCE_DT_INST_DEFINE(inst)                                                 \
	static struct stepper_timing_source stepper_timing_source_##inst;                          \
	IF_ENABLED(STEPPER_TIMING_DT_INST_USE_COUNTER(inst),					   \
		(STEPPER_TIMING_SOURCE_DT_INST_COUNTER_CFG_DEFINE(inst));			   \
	)                             \
	COND_CODE_1(STEPPER_TIMING_DT_INST_USE_COUNTER(inst),					   \
		(STEPPER_TIMING_SOURCE_DT_INST_COUNTER_DATA_DEFINE(inst)),			   \
		(STEPPER_TIMING_SOURCE_DT_INST_WORK_DATA_DEFINE(inst));				   \
	)                             \
	static struct stepper_timing_source stepper_timing_source_##inst = {                       \
		.config = COND_CODE_1(STEPPER_TIMING_DT_INST_USE_COUNTER(inst),			   \
			(&stepper_timing_source_cfg_##inst),					   \
			(NULL)),								   \
		.data = &stepper_timing_source_data_##inst,                                        \
	};

#define STEPPER_TIMING_SOURCE_DT_INST_GET(inst) (&stepper_timing_source_##inst)

#define STEPPER_TIMING_SOURCE_DT_SPEC_GET_API(inst)                                                \
	COND_CODE_1(STEPPER_TIMING_DT_INST_USE_COUNTER(inst),					   \
		(&stepper_timing_source_counter_api),						   \
		(&stepper_timing_source_work_api))

/**
 * @brief Initialize the stepper timing source.
 *
 * @param timing_source Pointer to the structure containing timing source config and data.
 *
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_init)(struct stepper_timing_source *timing_source);

/**
 * @brief Start the stepper timing source.
 *
 * @param timing_source Pointer to the structure containing timing source config and data.
 * @param interval time interval in nanoseconds after which the callback function should be
 * triggered.
 *
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_start)(const struct stepper_timing_source *timing_source,
					   uint64_t interval);

/**
 * @brief Stop the stepper timing source.
 *
 * @param timing_source Pointer to the structure containing timing source config and data.
 *
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_stop)(const struct stepper_timing_source *timing_source);

/**
 * Get current stepping interval.
 *
 * @param timing_source Pointer to the structure containing timing source config and data.
 *
 * @return timing interval in nanoseconds or 0 if the timing source is not running.
 */
typedef uint64_t (*stepper_timing_source_get_interval)(
	const struct stepper_timing_source *timing_source);

/**
 * @brief Stepper timing source API.
 */
struct stepper_timing_source_api {
	stepper_timing_source_init init;
	stepper_timing_source_start start;
	stepper_timing_source_stop stop;
	stepper_timing_source_get_interval get_interval;
};

extern const struct stepper_timing_source_api stepper_timing_source_work_api;
extern const struct stepper_timing_source_api stepper_timing_source_counter_api;

#endif /* ZEPHYR_DRIVER_STEPPER_MOTION_CONTROLLER_TIMING_SOURCE_H_ */
