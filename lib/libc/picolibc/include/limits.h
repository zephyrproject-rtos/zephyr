/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_LIMITS_H_
#define ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_LIMITS_H_

#include_next <limits.h>

#if defined(_POSIX_C_SOURCE)
#define GETENTROPY_MAX 256
#endif

#endif /* ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_LIMITS_H_ */
