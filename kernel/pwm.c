/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pwm.h>

int _impl_pwm_pin_set_cycles(struct device *dev, u32_t pwm,
			     u32_t period, u32_t pulse)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->pin_set(dev, pwm, period, pulse);
}

int _impl_pwm_get_cycles_per_sec(struct device *dev, u32_t pwm,
				 u64_t *cycles)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->get_cycles_per_sec(dev, pwm, cycles);
}
