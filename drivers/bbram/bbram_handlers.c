/*
 * Copyright (c) 2023, Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/bbram.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_bbram_check_invalid(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	return z_impl_bbram_check_invalid(dev);
}
#include <zephyr/syscalls/bbram_check_invalid_mrsh.c>

static inline int z_vrfy_bbram_check_standby_power(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	return z_impl_bbram_check_standby_power(dev);
}
#include <zephyr/syscalls/bbram_check_standby_power_mrsh.c>

static inline int z_vrfy_bbram_check_power(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	return z_impl_bbram_check_power(dev);
}
#include <zephyr/syscalls/bbram_check_power_mrsh.c>

static inline int z_vrfy_bbram_get_size(const struct device *dev, size_t *size)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(size, sizeof(size_t)));
	return z_impl_bbram_get_size(dev, size);
}
#include <zephyr/syscalls/bbram_get_size_mrsh.c>

static inline int z_vrfy_bbram_read(const struct device *dev, size_t offset,
				    size_t size, uint8_t *data)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, size));
	return z_impl_bbram_read(dev, offset, size, data);
}
#include <zephyr/syscalls/bbram_read_mrsh.c>

static inline int z_vrfy_bbram_write(const struct device *dev, size_t offset,
				     size_t size, const uint8_t *data)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_BBRAM));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, size));
	return z_impl_bbram_write(dev, offset, size, data);
}
#include <zephyr/syscalls/bbram_write_mrsh.c>
