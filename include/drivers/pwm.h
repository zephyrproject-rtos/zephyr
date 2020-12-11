/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public PWM Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_H_
#define ZEPHYR_INCLUDE_DRIVERS_PWM_H_

/**
 * @brief PWM Interface
 * @defgroup pwm_interface PWM Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <dt-bindings/pwm/pwm.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Provides a type to hold PWM configuration flags.
 */
typedef uint8_t pwm_flags_t;

/**
 * @typedef pwm_pin_set_t
 * @brief Callback API upon setting the pin
 * See @a pwm_pin_set_cycles() for argument description
 */
typedef int (*pwm_pin_set_t)(const struct device *dev, uint32_t pwm,
			     uint32_t period_cycles, uint32_t pulse_cycles,
			     pwm_flags_t flags);

/**
 * @typedef pwm_get_cycles_per_sec_t
 * @brief Callback API upon getting cycles per second
 * See @a pwm_get_cycles_per_sec() for argument description
 */
typedef int (*pwm_get_cycles_per_sec_t)(const struct device *dev,
					uint32_t pwm,
					uint64_t *cycles);

/** @brief PWM driver API definition. */
__subsystem struct pwm_driver_api {
	pwm_pin_set_t pin_set;
	pwm_get_cycles_per_sec_t get_cycles_per_sec;
};

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * Passing 0 as @p pulse will cause the pin to be driven to a constant
 * inactive level.
 * Passing a non-zero @p pulse equal to @p period will cause the pin
 * to be driven to a constant active level.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in clock cycle) set to the PWM. HW specific.
 * @param pulse Pulse width (in clock cycle) set to the PWM. HW specific.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int pwm_pin_set_cycles(const struct device *dev, uint32_t pwm,
				 uint32_t period, uint32_t pulse, pwm_flags_t flags);

static inline int z_impl_pwm_pin_set_cycles(const struct device *dev,
					    uint32_t pwm,
					    uint32_t period, uint32_t pulse,
					    pwm_flags_t flags)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->pin_set(dev, pwm, period, pulse, flags);
}

/**
 * @brief Get the clock rate (cycles per second) for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param cycles Pointer to the memory to store clock rate (cycles per sec).
 *		 HW specific.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */

__syscall int pwm_get_cycles_per_sec(const struct device *dev, uint32_t pwm,
				     uint64_t *cycles);

static inline int z_impl_pwm_get_cycles_per_sec(const struct device *dev,
						uint32_t pwm,
						uint64_t *cycles)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->get_cycles_per_sec(dev, pwm, cycles);
}

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in microseconds) set to the PWM.
 * @param pulse Pulse width (in microseconds) set to the PWM.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_usec(const struct device *dev, uint32_t pwm,
				   uint32_t period, uint32_t pulse,
				   pwm_flags_t flags)
{
	uint64_t period_cycles, pulse_cycles, cycles_per_sec;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / USEC_PER_SEC;
	if (period_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / USEC_PER_SEC;
	if (pulse_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return pwm_pin_set_cycles(dev, pwm, (uint32_t)period_cycles,
				  (uint32_t)pulse_cycles, flags);
}

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in nanoseconds) set to the PWM.
 * @param pulse Pulse width (in nanoseconds) set to the PWM.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_nsec(const struct device *dev, uint32_t pwm,
				   uint32_t period, uint32_t pulse,
				   pwm_flags_t flags)
{
	uint64_t period_cycles, pulse_cycles, cycles_per_sec;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / NSEC_PER_SEC;
	if (period_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / NSEC_PER_SEC;
	if (pulse_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return pwm_pin_set_cycles(dev, pwm, (uint32_t)period_cycles,
				  (uint32_t)pulse_cycles, flags);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/pwm.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_H_ */
