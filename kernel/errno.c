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

/*
 * Define _k_neg_eagain for use in assembly files as errno.h is
 * not assembly language safe.
 * FIXME: wastes 4 bytes
 */
const int _k_neg_eagain = -EAGAIN;

#ifdef CONFIG_ERRNO
int *_get_errno(void)
{
	return &_current->errno_var;
}
#endif
