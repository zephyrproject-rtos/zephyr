/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "picolibc-hooks.h"

#ifndef CONFIG_LIBC_ERRNO

/*
 * Picolibc needs to be able to declare this itself so that the library
 * doesn't end up needing zephyr header files. That means using a regular
 * function instead of an inline.
 */
int *z_errno_wrap(void)
{
	return z_errno();
}

#endif
