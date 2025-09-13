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

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_enable, const struct device *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_disable, const struct device *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_set_micro_step_res, const struct device *,
			enum stepper_drv_micro_step_resolution);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_get_micro_step_res, const struct device *,
			enum stepper_drv_micro_step_resolution *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_drv_set_event_callback, const struct device *,
			stepper_drv_event_cb_t, void *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_move_by, const struct device *, uint8_t, int32_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_set_microstep_interval, const struct device *, uint8_t,
			uint64_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_set_reference_position, const struct device *, uint8_t,
			int32_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_get_actual_position, const struct device *, uint8_t,
			int32_t *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_move_to, const struct device *, uint8_t, int32_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_is_moving, const struct device *, uint8_t, bool *);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_run, const struct device *, uint8_t,
			enum stepper_direction);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_stop, const struct device *, uint8_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_stepper_set_event_callback, const struct device *, uint8_t,
			stepper_event_callback_t, void *);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_FAKE_H_ */
