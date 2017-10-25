/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public PWM Driver APIs
 */

#ifndef __PWM_H__
#define __PWM_H__

/**
 * @brief PWM Interface
 * @defgroup pwm_interface PWM Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_ACCESS_BY_PIN	0
#define PWM_ACCESS_ALL		1

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

/**
 * @typedef pwm_pin_set_t
 * @brief Callback API upon setting the pin
 * See @a pwm_pin_set_cycles() for argument description
 */
typedef int (*pwm_pin_set_t)(struct device *dev, u32_t pwm,
			     u32_t period_cycles, u32_t pulse_cycles);

/**
 * @typedef pwm_get_cycles_per_sec_t
 * @brief Callback API upon getting cycles per second
 * See @a pwm_get_cycles_per_sec() for argument description
 */
typedef int (*pwm_get_cycles_per_sec_t)(struct device *dev, u32_t pwm,
					u64_t *cycles);

/** @brief PWM driver API definition. */
struct pwm_driver_api {
	pwm_pin_set_t pin_set;
	pwm_get_cycles_per_sec_t get_cycles_per_sec;
};

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in clock cycle) set to the PWM. HW specific.
 * @param pulse Pulse width (in clock cycle) set to the PWM. HW specific.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int pwm_pin_set_cycles(struct device *dev, u32_t pwm,
				 u32_t period, u32_t pulse);

static inline int _impl_pwm_pin_set_cycles(struct device *dev, u32_t pwm,
					   u32_t period, u32_t pulse)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->pin_set(dev, pwm, period, pulse);
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

__syscall int pwm_get_cycles_per_sec(struct device *dev, u32_t pwm,
				     u64_t *cycles);

static inline int _impl_pwm_get_cycles_per_sec(struct device *dev, u32_t pwm,
					       u64_t *cycles)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->get_cycles_per_sec(dev, pwm, cycles);
}

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in micro second) set to the PWM.
 * @param pulse Pulse width (in micro second) set to the PWM.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_usec(struct device *dev, u32_t pwm,
				   u32_t period, u32_t pulse)
{
	u64_t period_cycles, pulse_cycles, cycles_per_sec;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / USEC_PER_SEC;
	if (period_cycles >= ((u64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / USEC_PER_SEC;
	if (pulse_cycles >= ((u64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return pwm_pin_set_cycles(dev, pwm, (u32_t)period_cycles,
				  (u32_t)pulse_cycles);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/pwm.h>

#endif /* __PWM_H__ */
