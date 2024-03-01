/*
 * Copyright © 2024 Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_STRING_H_
#define ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_STRING_H_

/* This should work on GCC and clang.
 *
 * If we need to support a toolchain without #include_next the CMake
 * infrastructure should be used to identify it and provide an
 * alternative solution.
 */
#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Define this Zephyr API when _POSIX_C_SOURCE is not set to expose
 * it from newlib
 */
#if !__MISC_VISIBLE && !__POSIX_VISIBLE
char *strtok_r(char *__restrict, const char *__restrict, char **__restrict);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_NEWLIB_INCLUDE_STRING_H_ */
