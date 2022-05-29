/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_TIME_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_TIME_H_

#include <stdint.h>
#include <zephyr/toolchain.h>
#include <sys/_types.h>
#include <sys/_timespec.h>

/* Minimal time.h to fulfill the requirements of certain libraries
 * like mbedTLS and to support time APIs.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

#if !defined(__time_t_defined)
#define __time_t_defined
typedef _TIME_T_ time_t;
#endif

#if !defined(__suseconds_t_defined)
#define __suseconds_t_defined
typedef _SUSECONDS_T_ suseconds_t;
#endif

/*
 * Conversion between civil time and UNIX time.  The companion
 * localtime() and inverse mktime() are not provided here since they
 * require access to time zone information.
 */
struct tm *gmtime(const time_t *timep);
struct tm *gmtime_r(const time_t *ZRESTRICT timep,
		    struct tm *ZRESTRICT result);

time_t time(time_t *tloc);

#ifdef CONFIG_POSIX_CLOCK
#define TIMER_ABSTIME 4
typedef enum {
	CLOCK_MONOTONIC,
	CLOCK_REALTIME,
} clockid_t;
typedef unsigned long timer_t;

struct itimerspec {
	struct timespec it_interval;
	struct timespec it_value;
};

int clock_gettime(clockid_t clockid, struct timespec *res);
int clock_settime(clockid_t clockid, const struct timespec *tp);

int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
int timer_delete(timer_t timerid);
int timer_gettime(timer_t timerid, struct itimerspec *its);
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
	struct itimerspec *ovalue);

int nanosleep(const struct timespec *req, struct timespec *rem);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDIO_H_ */
