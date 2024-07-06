/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/retained_mem.h>
#include <zephyr/internal/syscall_handler.h>

static inline k_ssize_t z_vrfy_retained_mem_size(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RETAINED_MEM));
	return z_impl_retained_mem_size(dev);
}
#include <zephyr/syscalls/retained_mem_size_mrsh.c>

static inline int z_vrfy_retained_mem_read(const struct device *dev, k_off_t offset,
					   uint8_t *buffer, size_t size)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RETAINED_MEM));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buffer, size));
	return z_impl_retained_mem_read(dev, offset, buffer, size);
}
#include <zephyr/syscalls/retained_mem_read_mrsh.c>

static inline int z_vrfy_retained_mem_write(const struct device *dev, k_off_t offset,
					    const uint8_t *buffer, size_t size)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RETAINED_MEM));
	K_OOPS(K_SYSCALL_MEMORY_READ(buffer, size));
	return z_impl_retained_mem_write(dev, offset, buffer, size);
}
#include <zephyr/syscalls/retained_mem_write_mrsh.c>

static inline int z_vrfy_retained_mem_clear(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RETAINED_MEM));
	return z_impl_retained_mem_clear(dev);
}
#include <zephyr/syscalls/retained_mem_clear_mrsh.c>
