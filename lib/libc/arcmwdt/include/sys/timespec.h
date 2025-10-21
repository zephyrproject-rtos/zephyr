/*
 * Copyright (c) 2019, 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_SYS_TIMESPEC_H_
#define ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_SYS_TIMESPEC_H_

#include <sys/_timespec.h>

#define _TIMESPEC_DECLARED

struct itimerspec {
	struct timespec it_interval;  /* Timer interval */
	struct timespec it_value;     /* Timer expiration */
};

#endif /* ZEPHYR_LIB_LIBC_ARCMWDT_INCLUDE_SYS_TIMESPEC_H_ */
