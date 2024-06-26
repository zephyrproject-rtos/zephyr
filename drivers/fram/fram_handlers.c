/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/fram.h>

static inline int z_vrfy_fram_read(const struct device *dev, uint16_t addr, uint8_t *data,
				   size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FRAM(dev, read));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, len));
	return z_impl_fram_read((const struct device *)dev, addr, (uint8_t *)data, len);
}
#include <syscalls/fram_read_mrsh.c>

static inline int z_vrfy_fram_write(const struct device *dev, uint16_t addr, const uint8_t *data,
				    size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_FRAM(dev, write));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, len));
	return z_impl_fram_write((const struct device *)dev, addr, (const uint8_t *)data, len);
}
#include <syscalls/fram_write_mrsh.c>
