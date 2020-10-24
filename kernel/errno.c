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
#include <kernel.h>
#include <errno.h>

/*
 * Define _k_neg_eagain for use in assembly files as errno.h is
 * not assembly language safe.
 * FIXME: wastes 4 bytes
 */
const int _k_neg_eagain = -EAGAIN;

#if defined(CONFIG_ERRNO) && defined (CONFIG_ERRNO_IN_TLS)
__thread int z_errno_var;
#endif

/* NOTE: located here to avoid include dependency loops between errno.h
 * and kernel.h
 */
int *z_errno(void)
{
#ifdef CONFIG_ERRNO_IN_TLS
	return &z_errno_var;
#else
	return &_current->errno_var;
#endif
}
