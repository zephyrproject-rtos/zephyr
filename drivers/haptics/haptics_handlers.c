/*
 * Copyright 2024 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/haptics.h>

static inline int z_vrfy_haptics_start_output(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_HAPTICS(dev, start_output));

	return z_impl_haptics_start_output(dev);
}

#include <syscalls/haptics_start_output_mrsh.c>

static inline int z_vrfy_haptics_stop_output(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_HAPTICS(dev, stop_output));

	z_impl_haptics_stop_output(dev);
}

#include <syscalls/haptics_stop_output_mrsh.c>
