/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/pwm.h>

static inline int z_vrfy_pwm_set_cycles(const struct device *dev,
					uint32_t channel, uint32_t period,
					uint32_t pulse, pwm_flags_t flags)
{
	K_OOPS(K_SYSCALL_DRIVER_PWM(dev, set_cycles));
	return z_impl_pwm_set_cycles((const struct device *)dev, channel,
					 period, pulse, flags);
}
#include <zephyr/syscalls/pwm_set_cycles_mrsh.c>

static inline int z_vrfy_pwm_get_cycles_per_sec(const struct device *dev,
						uint32_t channel,
						uint64_t *cycles)
{
	K_OOPS(K_SYSCALL_DRIVER_PWM(dev, get_cycles_per_sec));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(cycles, sizeof(uint64_t)));
	return z_impl_pwm_get_cycles_per_sec((const struct device *)dev,
					     channel, (uint64_t *)cycles);
}
#include <zephyr/syscalls/pwm_get_cycles_per_sec_mrsh.c>

#ifdef CONFIG_PWM_CAPTURE

static inline int z_vrfy_pwm_enable_capture(const struct device *dev,
					    uint32_t channel)
{
	K_OOPS(K_SYSCALL_DRIVER_PWM(dev, enable_capture));
	return z_impl_pwm_enable_capture((const struct device *)dev, channel);
}
#include <zephyr/syscalls/pwm_enable_capture_mrsh.c>

static inline int z_vrfy_pwm_disable_capture(const struct device *dev,
					     uint32_t channel)
{
	K_OOPS(K_SYSCALL_DRIVER_PWM(dev, disable_capture));
	return z_impl_pwm_disable_capture((const struct device *)dev, channel);
}
#include <zephyr/syscalls/pwm_disable_capture_mrsh.c>

static inline int z_vrfy_pwm_capture_cycles(const struct device *dev,
					    uint32_t channel, pwm_flags_t flags,
					    uint32_t *period_cycles,
					    uint32_t *pulse_cycles,
					    k_timeout_t timeout)
{
	uint32_t period;
	uint32_t pulse;
	int err;

	K_OOPS(K_SYSCALL_DRIVER_PWM(dev, configure_capture));
	K_OOPS(K_SYSCALL_DRIVER_PWM(dev, enable_capture));
	K_OOPS(K_SYSCALL_DRIVER_PWM(dev, disable_capture));

	err = z_impl_pwm_capture_cycles((const struct device *)dev, channel,
					flags, &period, &pulse, timeout);
	if (period_cycles != NULL) {
		K_OOPS(k_usermode_to_copy(period_cycles, &period,
				      sizeof(*period_cycles)));
	}

	if (pulse_cycles != NULL) {
		K_OOPS(k_usermode_to_copy(pulse_cycles, &pulse,
				      sizeof(*pulse_cycles)));
	}

	return err;
}
#include <zephyr/syscalls/pwm_capture_cycles_mrsh.c>

#endif /* CONFIG_PWM_CAPTURE */
