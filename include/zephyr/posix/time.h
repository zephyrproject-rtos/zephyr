/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_TIME_H_
#define ZEPHYR_INCLUDE_POSIX_TIME_H_

#ifdef CONFIG_PICOLIBC
/* temporary workaround for https://github.com/picolibc/picolibc/pull/1079 */
#include <sys/_types.h>

#ifndef _PID_T_DECLARED
typedef __pid_t pid_t;
#define _PID_T_DECLARED
#endif
#endif

/* This header will soon be deprecated. Please include <time.h> instead. */
#include_next <time.h>

#endif /* ZEPHYR_INCLUDE_POSIX_TIME_H_ */
