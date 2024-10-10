/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_FAKE_H_

#include <zephyr/drivers/stepper.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_enable, const struct device *, const bool);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_move, const struct device *, const int32_t,
			struct k_poll_signal *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_set_max_velocity, const struct device *, const uint32_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_set_micro_step_res, const struct device *,
			const enum micro_step_resolution);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_get_micro_step_res, const struct device *,
			enum micro_step_resolution *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_set_actual_position, const struct device *,
			const int32_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_get_actual_position, const struct device *, int32_t *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_set_target_position, const struct device *, const int32_t,
			struct k_poll_signal *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_is_moving, const struct device *, bool *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_enable_constant_velocity_mode, const struct device *,
			const enum stepper_direction, const uint32_t);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_FAKE_H_ */
