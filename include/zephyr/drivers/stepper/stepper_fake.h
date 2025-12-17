/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_FAKE_H_

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper_motor.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_driver_enable, const struct device *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_driver_disable, const struct device *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_driver_set_micro_step_res, const struct device *,
			enum stepper_micro_step_resolution);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_driver_get_micro_step_res, const struct device *,
			enum stepper_micro_step_resolution *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_driver_set_event_cb, const struct device *,
			stepper_event_cb_t, void *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_motor_move_by, const struct device *, int32_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_motor_set_microstep_interval, const struct device *,
			uint64_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_motor_set_reference_position, const struct device *,
			int32_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_motor_get_actual_position, const struct device *,
			int32_t *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_motor_move_to, const struct device *, int32_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_motor_is_moving, const struct device *, bool *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_motor_run, const struct device *,
			enum stepper_motor_direction);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_motor_stop, const struct device *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_motor_set_event_callback, const struct device *,
			stepper_motor_event_callback_t, void *);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_FAKE_H_ */
