/*
 * Copyright (c) 2023 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_LIMITS_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_LIMITS_H_

#include_next <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#include <zephyr/posix/posix_limits.h>

#else

#define PATH_MAX 256

#endif

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_LIMITS_H_ */
