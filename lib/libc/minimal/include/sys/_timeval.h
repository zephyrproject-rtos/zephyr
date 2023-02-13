/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS__TIMEVAL_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS__TIMEVAL_H_

#include <sys/_types.h>

#if !defined(__time_t_defined)
#define __time_t_defined
typedef _TIME_T_ time_t;
#endif

#if !defined(__suseconds_t_defined)
#define __suseconds_t_defined
typedef _SUSECONDS_T_ suseconds_t;
#endif

struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS__TIMEVAL_H_ */
