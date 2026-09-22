/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_LIMITS_H_
#define ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_LIMITS_H_

#include_next <limits.h>
#ifdef CONFIG_NEWLIB_LIBC_USE_POSIX_LIMITS_H
#include <zephyr/posix/posix_limits.h>
#endif

#endif /* ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_LIMITS_H_ */
