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
 * The fake PWM driver implements every PWM API callback as a Fake Function
 * Framework (FFF) fake. It is enabled by @kconfig{CONFIG_PWM_FAKE} and
 * instantiated from @dtcompatible{zephyr,fake-pwm} devicetree nodes.
 *
 * Each fake is named after the API function it backs (`fake_pwm_set_cycles()`
 * for `pwm_set_cycles()`) and is paired with an FFF control structure carrying
 * an additional `_fake` suffix (`fake_pwm_set_cycles_fake`). Test suites
 * include this header to set return values, install custom fakes, or inspect
 * call counts and captured arguments. See @rstref{mocking-fff}.
 *
 * When @kconfig{CONFIG_ZTEST} is enabled, a ztest rule resets all fakes before
 * each test case.
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

/** @cond INTERNAL_HIDDEN */

DECLARE_FAKE_VALUE_FUNC(int, fake_pwm_set_cycles, const struct device *, uint32_t, uint32_t,
			uint32_t, pwm_flags_t);

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_PWM_FAKE_H_ */
