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

#include <kernel_structs.h>
#include <syscall_handler.h>

/*
 * Define _k_neg_eagain for use in assembly files as errno.h is
 * not assembly language safe.
 * FIXME: wastes 4 bytes
 */
const int _k_neg_eagain = -EAGAIN;

#ifdef CONFIG_ERRNO
#ifdef CONFIG_USERSPACE
int *_impl_z_errno(void)
{
	/* Initialized to the lowest address in the stack so the thread can
	 * directly read/write it
	 */
	return &_current->userspace_local_data->errno_var;
}

Z_SYSCALL_HANDLER0_SIMPLE(z_errno);
#else
int *_impl_z_errno(void)
{
	return &_current->errno_var;
}
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_ERRNO */
