/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cache.h>
#include <syscall_handler.h>

static inline void z_vrfy_sys_cache_flush(void *addr, size_t size)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(addr, size));
	z_impl_sys_cache_flush(addr, size);
}
#include <syscalls/sys_cache_flush_mrsh.c>

static inline void z_vrfy_sys_cache_invd(void *addr, size_t size)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(addr, size));
	z_impl_sys_cache_invd(addr, size);
}
#include <syscalls/sys_cache_invd_mrsh.c>
