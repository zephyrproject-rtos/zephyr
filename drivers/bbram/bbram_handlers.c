/*
 * Copyright (c) 2023, Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/bbram.h>
#include <zephyr/syscall_handler.h>

static inline int z_vrfy_bbram_check_invalid(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	return z_impl_bbram_check_invalid(dev);
}
#include <syscalls/bbram_check_invalid_mrsh.c>

static inline int z_vrfy_bbram_check_standby_power(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	return z_impl_bbram_check_standby_power(dev);
}
#include <syscalls/bbram_check_standby_power_mrsh.c>

static inline int z_vrfy_bbram_check_power(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	return z_impl_bbram_check_power(dev);
}
#include <syscalls/bbram_check_power_mrsh.c>

static inline int z_vrfy_bbram_get_size(const struct device *dev, size_t *size)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(size, sizeof(size_t)));
	return z_impl_bbram_get_size(dev, size);
}
#include <syscalls/bbram_get_size_mrsh.c>

static inline int z_vrfy_bbram_read(const struct device *dev, size_t offset,
				    size_t size, uint8_t *data)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, size));
	return z_impl_bbram_read(dev, offset, size, data);
}
#include <syscalls/bbram_read_mrsh.c>

static inline int z_vrfy_bbram_write(const struct device *dev, size_t offset,
				     size_t size, const uint8_t *data)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, size));
	return z_impl_bbram_write(dev, offset, size, data);
}
#include <syscalls/bbram_write_mrsh.c>
