/*
 * Copyright (c) 2024, Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_DRIVERS_PWM_PWM_FAKE_H_
#define INCLUDE_DRIVERS_PWM_PWM_FAKE_H_

#include <zephyr/drivers/pwm.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int, fake_pwm_set_cycles, const struct device *, uint32_t, uint32_t,
			uint32_t, pwm_flags_t);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_PWM_PWM_FAKE_H_ */
