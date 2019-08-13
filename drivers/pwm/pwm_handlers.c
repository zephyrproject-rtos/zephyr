/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/pwm.h>

static inline int z_vrfy_pwm_pin_set_cycles(struct device *dev, u32_t pwm,
					   u32_t period, u32_t pulse)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, pin_set));
	return z_impl_pwm_pin_set_cycles((struct device *)dev, pwm, period,
					pulse);
}
#include <syscalls/pwm_pin_set_cycles_mrsh.c>

static inline int z_vrfy_pwm_get_cycles_per_sec(struct device *dev, u32_t pwm,
					       u64_t *cycles)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, get_cycles_per_sec));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(cycles, sizeof(u64_t)));
	return z_impl_pwm_get_cycles_per_sec((struct device *)dev,
					    pwm, (u64_t *)cycles);
}
#include <syscalls/pwm_get_cycles_per_sec_mrsh.c>
