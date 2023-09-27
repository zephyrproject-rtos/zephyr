/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/entropy.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_entropy_get_entropy(const struct device *dev,
					     uint8_t *buffer,
					     uint16_t len)
{
	K_OOPS(K_SYSCALL_DRIVER_ENTROPY(dev, get_entropy));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buffer, len));
	return z_impl_entropy_get_entropy((const struct device *)dev,
					  (uint8_t *)buffer,
					  len);
}
#include <syscalls/entropy_get_entropy_mrsh.c>
