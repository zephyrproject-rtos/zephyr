/*
 * Copyright (c) 2019 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_SYS_TIMESPEC_H_
#define ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_SYS_TIMESPEC_H_

#include <sys/types.h>

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

struct itimerspec {
	struct timespec it_interval;  /* Timer interval */
	struct timespec it_value;     /* Timer expiration */
};

#endif /* ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_SYS_TIMESPEC_H_ */
