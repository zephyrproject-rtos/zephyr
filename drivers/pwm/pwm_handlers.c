/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <pwm.h>

_SYSCALL_HANDLER(pwm_pin_set_cycles, dev, pwm, period, pulse)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PWM);
	return _impl_pwm_pin_set_cycles((struct device *)dev, pwm, period,
					pulse);
}

_SYSCALL_HANDLER(pwm_get_cycles_per_sec, dev, pwm, cycles)
{
	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_PWM);
	_SYSCALL_MEMORY_WRITE(cycles, sizeof(u64_t));
	return _impl_pwm_get_cycles_per_sec((struct device *)dev,
					    pwm, (u64_t *)cycles);
}
