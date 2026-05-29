/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_sys_cache_data_flush_range(void *addr, size_t size)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(addr, size));

	return z_impl_sys_cache_data_flush_range(addr, size);
}
#include <zephyr/syscalls/sys_cache_data_flush_range_mrsh.c>

static inline int z_vrfy_sys_cache_data_invd_range(void *addr, size_t size)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(addr, size));

	return z_impl_sys_cache_data_invd_range(addr, size);
}
#include <zephyr/syscalls/sys_cache_data_invd_range_mrsh.c>

static inline int z_vrfy_sys_cache_data_flush_and_invd_range(void *addr, size_t size)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(addr, size));

	return z_impl_sys_cache_data_flush_and_invd_range(addr, size);
}
#include <zephyr/syscalls/sys_cache_data_flush_and_invd_range_mrsh.c>
