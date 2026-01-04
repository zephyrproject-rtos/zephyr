/*
 * Copyright (c) 2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/actuator.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_actuator_set_setpoint(const struct device *dev, q31_t setpoint)
{
	K_OOPS(K_SYSCALL_DRIVER_ACTUATOR(dev, get_output));
	return z_impl_actuator_set_setpoint(dev);
}
#include <zephyr/syscalls/actuator_set_setpoint_mrsh.c>
