/*
 * Copyright (c) 2024, Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake PWM driver API functions.
 * @ingroup pwm_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_PWM_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_PWM_PWM_FAKE_H_

#include <zephyr/drivers/pwm.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake PWM driver
 * @defgroup pwm_fake Fake PWM
 * @ingroup io_emulators
 * @ingroup pwm_interface
 *
 * @driver_fake{pwm_interface,CONFIG_PWM_FAKE,zephyr\,fake-pwm}
 *
 * @code{.c}
 * const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fake_pwm));
 *
 * zassert_ok(pwm_set_cycles(dev, 0, 1000, 500, 0));
 *
 * zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
 * zassert_equal(1000, fake_pwm_set_cycles_fake.arg2_val);
 * zassert_equal(500, fake_pwm_set_cycles_fake.arg3_val);
 * @endcode
 *
 * @{
 */

/** @fake_of{pwm_driver_api::set_cycles} */
DECLARE_FAKE_VALUE_FUNC(int, fake_pwm_set_cycles, const struct device *, uint32_t, uint32_t,
			uint32_t, pwm_flags_t);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_PWM_FAKE_H_ */
