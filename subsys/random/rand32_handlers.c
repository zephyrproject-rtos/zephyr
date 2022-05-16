/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/random/rand32.h>
#include <zephyr/syscall_handler.h>


static inline uint32_t z_vrfy_sys_rand32_get(void)
{
	return z_impl_sys_rand32_get();
}
#include <syscalls/sys_rand32_get_mrsh.c>

static inline void z_vrfy_sys_rand_get(void *dst, size_t len)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(dst, len));

	z_impl_sys_rand_get(dst, len);
}
#include <syscalls/sys_rand_get_mrsh.c>

#if defined(CONFIG_CTR_DRBG_CSPRNG_GENERATOR) || \
	defined(CONFIG_HARDWARE_DEVICE_CS_GENERATOR)
static inline int z_vrfy_sys_csrand_get(void *dst, size_t len)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(dst, len));

	return z_impl_sys_csrand_get(dst, len);
}
#include <syscalls/sys_csrand_get_mrsh.c>
#endif
