/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_RAMP_CONSTANT_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_RAMP_CONSTANT_H_

struct stepper_ramp_constant_data {
	uint64_t interval_ns;
	uint32_t steps_left;
};

struct stepper_ramp_constant_profile {
	uint64_t interval_ns;
};

struct stepper_ramp_constant {
	const struct stepper_ramp_api *const api;
	struct stepper_ramp_constant_data *const data;
	struct stepper_ramp_constant_profile *const profile;
};

#ifdef CONFIG_STEPPER_RAMP_CONSTANT
extern const struct stepper_ramp_api stepper_ramp_constant_api;

#define STEPPER_RAMP_CONSTANT_DEFINE(name, interval)                                               \
	struct stepper_ramp_constant_profile name##_profile = {                                    \
		.interval_ns = interval,                                                           \
	};                                                                                         \
	struct stepper_ramp_constant_data name##_data = {                                          \
		.interval_ns = interval,                                                           \
		.steps_left = 0,                                                                   \
	};                                                                                         \
	struct stepper_ramp_constant name##_ramp = {                                               \
		.api = &stepper_ramp_constant_api,                                                 \
		.data = &name##_data,                                                              \
		.profile = &name##_profile,                                                        \
	};                                                                                         \
	struct stepper_ramp_base *name = (struct stepper_ramp_base *)&name##_ramp;

#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_RAMP_CONSTANT_H_ */
