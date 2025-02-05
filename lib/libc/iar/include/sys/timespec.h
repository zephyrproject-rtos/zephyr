/*
 * Copyright (c) 2025 IAR Systems AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_IAR_INCLUDE_SYS_TIMESPEC_H_
#define ZEPHYR_LIB_LIBC_IAR_INCLUDE_SYS_TIMESPEC_H_

#include <sys/types.h>
#include <sys/_timespec.h>

struct itimerspec {
	struct timespec it_interval;  /* Timer interval */
	struct timespec it_value;     /* Timer expiration */
};

#endif /* ZEPHYR_LIB_LIBC_IAR_INCLUDE_SYS_TIMESPEC_H_ */
