/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_TIMING_SOURCE_H_
#define ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_TIMING_SOURCE_H_

#include <zephyr/device.h>

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
 * @param velocity Velocity in microsteps per second.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_update)(const struct device *dev, uint32_t velocity);

/**
 * @brief Start the stepper timing source.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
typedef int (*stepper_timing_source_start)(const struct device *dev);

/**
 * @brief Whether the stepper timing source requires rescheduling (keeps running
 *        after the initial start).
 *
 * @param dev Pointer to the device structure.
 * @return true if the timing source requires rescheduling, false otherwise.
 */
typedef bool (*stepper_timing_sources_requires_reschedule)(const struct device *dev);

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
 * @brief Stepper timing source API.
 */
struct stepper_timing_source_api {
	stepper_timing_source_init init;
	stepper_timing_source_update update;
	stepper_timing_source_start start;
	stepper_timing_sources_requires_reschedule needs_reschedule;
	stepper_timing_source_stop stop;
	stepper_timing_source_is_running is_running;
};

extern const struct stepper_timing_source_api step_work_timing_source_api;
#ifdef CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING
extern const struct stepper_timing_source_api step_counter_timing_source_api;
#endif /* CONFIG_STEP_DIR_STEPPER_COUNTER_TIMING */

#endif /* ZEPHYR_DRIVER_STEPPER_STEP_DIR_STEPPER_TIMING_SOURCE_H_ */
