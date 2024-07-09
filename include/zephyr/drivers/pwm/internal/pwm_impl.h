/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementations of PWM Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_H_
#error "Should only be included by zephyr/drivers/pwm.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_INTERNAL_PWM_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_PWM_INTERNAL_PWM_IMPL_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_pwm_set_cycles(const struct device *dev,
					uint32_t channel, uint32_t period,
					uint32_t pulse, pwm_flags_t flags)
{
	const struct pwm_driver_api *api =
		(const struct pwm_driver_api *)dev->api;

	if (pulse > period) {
		return -EINVAL;
	}

	return api->set_cycles(dev, channel, period, pulse, flags);
}

static inline int z_impl_pwm_get_cycles_per_sec(const struct device *dev,
						uint32_t channel,
						uint64_t *cycles)
{
	const struct pwm_driver_api *api =
		(const struct pwm_driver_api *)dev->api;

	return api->get_cycles_per_sec(dev, channel, cycles);
}

#ifdef CONFIG_PWM_CAPTURE
static inline int z_impl_pwm_enable_capture(const struct device *dev,
					    uint32_t channel)
{
	const struct pwm_driver_api *api =
		(const struct pwm_driver_api *)dev->api;

	if (api->enable_capture == NULL) {
		return -ENOSYS;
	}

	return api->enable_capture(dev, channel);
}
#endif /* CONFIG_PWM_CAPTURE */

#ifdef CONFIG_PWM_CAPTURE
static inline int z_impl_pwm_disable_capture(const struct device *dev,
					     uint32_t channel)
{
	const struct pwm_driver_api *api =
		(const struct pwm_driver_api *)dev->api;

	if (api->disable_capture == NULL) {
		return -ENOSYS;
	}

	return api->disable_capture(dev, channel);
}
#endif /* CONFIG_PWM_CAPTURE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_INTERNAL_PWM_IMPL_H_ */
