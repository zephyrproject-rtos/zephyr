/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include "sample_driver.h"

int z_vrfy_sample_driver_state_set(const struct device *dev, bool active)
{
	if (K_SYSCALL_DRIVER_SAMPLE(dev, state_set)) {
		return -EINVAL;
	}

	return z_impl_sample_driver_state_set(dev, active);
}

#include <syscalls/sample_driver_state_set_mrsh.c>

int z_vrfy_sample_driver_write(const struct device *dev, void *buf)
{
	if (K_SYSCALL_DRIVER_SAMPLE(dev, write)) {
		return -EINVAL;
	}

	if (K_SYSCALL_MEMORY_READ(buf, SAMPLE_DRIVER_MSG_SIZE)) {
		return -EFAULT;
	}

	return z_impl_sample_driver_write(dev, buf);
}
#include <syscalls/sample_driver_write_mrsh.c>
