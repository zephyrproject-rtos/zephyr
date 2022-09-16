/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/syscall_handler.h>

static inline int z_vrfy_sys_cache_data_all(int op)
{
	return z_impl_sys_cache_data_all(op);
}
#include <syscalls/sys_cache_data_all_mrsh.c>

static inline int z_vrfy_sys_cache_data_range(void *addr, size_t size, int op)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(addr, size));

	return z_impl_sys_cache_data_range(addr, size, op);
}
#include <syscalls/sys_cache_data_range_mrsh.c>

static inline int z_vrfy_sys_cache_instr_all(int op)
{
	return z_impl_sys_cache_instr_all(op);
}
#include <syscalls/sys_cache_instr_all_mrsh.c>

static inline int z_vrfy_sys_cache_instr_range(void *addr, size_t size, int op)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(addr, size));

	return z_impl_sys_cache_instr_range(addr, size, op);
}
#include <syscalls/sys_cache_instr_range_mrsh.c>
