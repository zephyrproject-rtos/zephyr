/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS__TIMEVAL_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS__TIMEVAL_H_

#include <sys/types.h>

struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS__TIMEVAL_H_ */
