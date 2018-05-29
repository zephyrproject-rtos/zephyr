/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <pwm.h>

Z_SYSCALL_HANDLER(pwm_pin_set_cycles, dev, pwm, period, pulse)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, pin_set));
	return _impl_pwm_pin_set_cycles((struct device *)dev, pwm, period,
					pulse);
}

Z_SYSCALL_HANDLER(pwm_get_cycles_per_sec, dev, pwm, cycles)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, get_cycles_per_sec));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(cycles, sizeof(u64_t)));
	return _impl_pwm_get_cycles_per_sec((struct device *)dev,
					    pwm, (u64_t *)cycles);
}
