/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *
 * @brief Per-thread errno accessor function
 *
 * Allow accessing the errno for the current thread without involving the
 * context switching.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

/*
 * Define _k_neg_eagain for use in assembly files as errno.h is
 * not assembly language safe.
 * FIXME: wastes 4 bytes
 */
const int _k_neg_eagain = -EAGAIN;

#ifdef CONFIG_ERRNO

#if defined(CONFIG_LIBC_ERRNO)
/* nothing needed here */
#elif defined(CONFIG_ERRNO_IN_TLS)
Z_THREAD_LOCAL int z_errno_var;
#else

#ifdef CONFIG_USERSPACE
int *z_impl_z_errno(void)
{
	/* Initialized to the lowest address in the stack so the thread can
	 * directly read/write it
	 */
	return &arch_current_thread()->userspace_local_data->errno_var;
}

static inline int *z_vrfy_z_errno(void)
{
	return z_impl_z_errno();
}
#include <zephyr/syscalls/z_errno_mrsh.c>

#else
int *z_impl_z_errno(void)
{
	return &arch_current_thread()->errno_var;
}
#endif /* CONFIG_USERSPACE */

#endif /* CONFIG_ERRNO_IN_TLS */

#endif /* CONFIG_ERRNO */
