/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/entropy.h>
#include <syscall_handler.h>

static inline int z_vrfy_entropy_get_entropy(struct device *dev,
					     u8_t *buffer,
					     u16_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_ENTROPY(dev, get_entropy));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buffer, len));
	return z_impl_entropy_get_entropy((struct device *)dev, (u8_t *)buffer,
					 len);
}
#include <syscalls/entropy_get_entropy_mrsh.c>
