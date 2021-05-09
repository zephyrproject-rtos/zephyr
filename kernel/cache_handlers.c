/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cache.h>
#include <syscall_handler.h>

static inline int z_vrfy_sys_dcache_all(int op)
{
	return z_impl_sys_dcache_all(op);
}
#include <syscalls/sys_dcache_all_mrsh.c>

static inline int z_vrfy_sys_dcache_range(void *addr, size_t size, int op)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(addr, size));

	return z_impl_sys_dcache_range(addr, size, op);
}
#include <syscalls/sys_dcache_range_mrsh.c>

static inline int z_vrfy_sys_icache_all(int op)
{
	return z_impl_sys_icache_all(op);
}
#include <syscalls/sys_icache_all_mrsh.c>

static inline int z_vrfy_sys_icache_range(void *addr, size_t size, int op)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(addr, size));

	return z_impl_sys_icache_range(addr, size, op);
}
#include <syscalls/sys_icache_range_mrsh.c>
