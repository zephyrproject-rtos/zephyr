/*
 * Copyright (c) 2025 IAR Systems AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_IAR_INCLUDE_SYS__TIMEVAL_H_
#define ZEPHYR_LIB_LIBC_IAR_INCLUDE_SYS__TIMEVAL_H_

#include <stdint.h>

#if !defined(__time_t_defined)
#define __time_t_defined
typedef long long time_t;
#endif

#if !defined(__suseconds_t_defined)
#define __suseconds_t_defined
typedef int32_t suseconds_t;
#endif

struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};

#endif /* ZEPHYR_LIB_LIBC_IAR_INCLUDE_SYS__TIMEVAL_H_ */
