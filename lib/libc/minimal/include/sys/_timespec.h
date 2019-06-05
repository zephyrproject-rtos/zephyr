/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS__TIMESPEC_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS__TIMESPEC_H_

#include <sys/types.h>

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS__TIMESPEC_H_ */
