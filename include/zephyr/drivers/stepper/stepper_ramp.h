/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_RAMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_RAMP_H_

#include <stdint.h>

/**
 * @brief Stepper ramp profile
 */
struct stepper_ramp_profile {
	/** Ramp acceleration in micro-steps per second squared */
	uint32_t acceleration;
	/** Ramp maximum velocity in micro-steps per second */
	uint32_t max_velocity;
	/** Ramp deceleration in micro-steps per second squared */
	uint32_t deceleration;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_RAMP_H_ */
