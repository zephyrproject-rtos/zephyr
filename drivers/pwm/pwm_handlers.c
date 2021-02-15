/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
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

#ifdef CONFIG_PWM_CAPTURE

static inline int z_vrfy_pwm_pin_enable_capture(const struct device *dev,
						uint32_t pwm)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, pin_enable_capture));
	return z_impl_pwm_pin_enable_capture((const struct device *)dev, pwm);
}
#include <syscalls/pwm_pin_enable_capture_mrsh.c>

static inline int z_vrfy_pwm_pin_disable_capture(const struct device *dev,
						 uint32_t pwm)
{
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, pin_disable_capture));
	return z_impl_pwm_pin_disable_capture((const struct device *)dev, pwm);
}
#include <syscalls/pwm_pin_disable_capture_mrsh.c>

static inline int z_vrfy_pwm_pin_capture_cycles(const struct device *dev,
						uint32_t pwm, pwm_flags_t flags,
						uint32_t *period_cycles,
						uint32_t *pulse_cycles,
						k_timeout_t timeout)
{
	uint32_t period;
	uint32_t pulse;
	int err;

	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, pin_configure_capture));
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, pin_enable_capture));
	Z_OOPS(Z_SYSCALL_DRIVER_PWM(dev, pin_disable_capture));

	err = z_impl_pwm_pin_capture_cycles((const struct device *)dev, pwm,
					    flags, &period, &pulse, timeout);
	if (period_cycles != NULL) {
		Z_OOPS(z_user_to_copy(period_cycles, &period,
				      sizeof(*period_cycles)));
	}

	if (pulse_cycles != NULL) {
		Z_OOPS(z_user_to_copy(pulse_cycles, &pulse,
				      sizeof(*pulse_cycles)));
	}

	return err;
}
#include <syscalls/pwm_pin_capture_cycles_mrsh.c>

#endif /* CONFIG_PWM_CAPTURE */
