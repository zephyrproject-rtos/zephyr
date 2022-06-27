/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_TIME_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_TIME_H_

#ifdef CONFIG_NEWLIB_LIBC
/* Kludge to support outdated newlib version as used in SDK 0.10 for Xtensa */
#include <newlib.h>

#ifdef __NEWLIB__
#include <sys/_timeval.h>
#else
#include <sys/types.h>
struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};
#endif

#else
#include <sys/_timeval.h>
#endif /* CONFIG_NEWLIB_LIBC */

#ifdef __cplusplus
extern "C" {
#endif

int gettimeofday(struct timeval *tv, const void *tz);

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_SYS_TIME_H_ */
