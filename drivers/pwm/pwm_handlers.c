/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/pwm.h>

static inline int z_vrfy_pwm_pin_set_cycles(const struct device *dev,
					    uint32_t pwm,
					    uint32_t period, uint32_t pulse,
					    pwm_flags_t flags)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, pin_set));
	return z_impl_pwm_pin_set_cycles((const struct device *)dev, pwm,
					 period,
					 pulse, flags);
}
#include <syscalls/pwm_pin_set_cycles_mrsh.c>

static inline int z_vrfy_pwm_get_cycles_per_sec(const struct device *dev,
						uint32_t pwm,
						uint64_t *cycles)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, get_cycles_per_sec));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(cycles, sizeof(uint64_t)));
	return z_impl_pwm_get_cycles_per_sec((const struct device *)dev,
					     pwm, (uint64_t *)cycles);
}
#include <syscalls/pwm_get_cycles_per_sec_mrsh.c>
