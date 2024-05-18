/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_TIME_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_TIME_H_

#include <stdint.h>
#include <sys/_types.h>
#include <sys/_timespec.h>

#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

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

#undef CLOCK_REALTIME
#define CLOCK_REALTIME ((clockid_t)1)

#if defined(_POSIX_CPUTIME) || defined(__DOXYGEN__)
#undef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID ((clockid_t)2)
#endif /* defined(_POSIX_CPUTIME) || defined(__DOXYGEN__) */

#if defined(_POSIX_MONOTONIC_CLOCK) || defined(__DOXYGEN__)
#undef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC ((clockid_t)4)
#endif

#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME 4
#endif

#if defined(_POSIX_TIMERS) || defined(__DOXYGEN__)

/* Timer APIs */
int clock_gettime(clockid_t clock_id, struct timespec *ts);
int clock_getres(clockid_t clock_id, struct timespec *ts);
int clock_settime(clockid_t clock_id, const struct timespec *ts);
int timer_create(clockid_t clockId, struct sigevent *ZRESTRICT evp, timer_t *ZRESTRICT timerid);
int timer_delete(timer_t timerid);
int timer_gettime(timer_t timerid, struct itimerspec *its);
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
		  struct itimerspec *ovalue);
int timer_getoverrun(timer_t timerid);
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

#endif /* defined(_POSIX_TIMERS) || defined(__DOXYGEN__) */

#if defined(_POSIX_CLOCK_SELECTION)
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
		    struct timespec *rmtp);
#endif /* defined(_POSIX_CLOCK_SELECTION) */

#if defined(_POSIX_CPUTIME) || defined(__DOXYGEN__)

#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID ((clockid_t)2)
#endif

int clock_getcpuclockid(pid_t pid, clockid_t *clock_id);

#endif /* defined(_POSIX_CPUTIME) || defined(__DOXYGEN__) */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_STDIO_H_ */
