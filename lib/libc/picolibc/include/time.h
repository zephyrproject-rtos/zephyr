/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_TIME_H_
#define ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_TIME_H_

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

/* temporary workaround for https://github.com/picolibc/picolibc/pull/1079 */
#include <sys/_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _PID_T_DECLARED
typedef __pid_t pid_t;
#define _PID_T_DECLARED
#endif

#ifdef __cplusplus
}
#endif

#endif /* defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__) */

#include_next <time.h>

#endif /* ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_TIME_H_ */
