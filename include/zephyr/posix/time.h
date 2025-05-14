/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_TIME_H_
#define ZEPHYR_INCLUDE_POSIX_TIME_H_

/* Read standard header.  This may find <posix/time.h> since they
 * refer to the same file when include/posix is in the search path.
 */
#include <time.h>

#ifdef CONFIG_NEWLIB_LIBC
/* Kludge to support outdated newlib version as used in SDK 0.10 for Xtensa */
#include <newlib.h>

#ifdef __NEWLIB__
/* Newever Newlib 3.x+ */
#include <sys/timespec.h>
#else /* __NEWLIB__ */
/* Workaround for older Newlib 2.x, as used by Xtensa. It lacks sys/_timeval.h,
 * so mimic it here.
 */
#include <sys/types.h>
#ifndef __timespec_defined

#ifdef __cplusplus
extern "C" {
#endif

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

struct itimerspec {
	struct timespec it_interval;  /* Timer interval */
	struct timespec it_value;     /* Timer expiration */
};

#ifdef __cplusplus
}
#endif

#endif /* __timespec_defined */
#endif /* __NEWLIB__ */

#else /* CONFIG_NEWLIB_LIBC */
/* Not Newlib */
# if defined(CONFIG_ARCH_POSIX) && defined(CONFIG_EXTERNAL_LIBC)
#  include <bits/types/struct_timespec.h>
#  include <bits/types/struct_itimerspec.h>
# else
#  include <sys/timespec.h>
# endif
#endif /* CONFIG_NEWLIB_LIBC */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/posix/posix_types.h>
#include <zephyr/posix/signal.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 1
#endif

#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID 2
#endif

#ifndef CLOCK_THREAD_CPUTIME_ID
#define CLOCK_THREAD_CPUTIME_ID 3
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 4
#endif

#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME 4
#endif

int clock_gettime(clockid_t clock_id, struct timespec *ts);
int clock_getres(clockid_t clock_id, struct timespec *ts);
int clock_settime(clockid_t clock_id, const struct timespec *ts);
int clock_getcpuclockid(pid_t pid, clockid_t *clock_id);
/* Timer APIs */
int timer_create(clockid_t clockId, struct sigevent *evp, timer_t *timerid);
int timer_delete(timer_t timerid);
struct itimerspec;
int timer_gettime(timer_t timerid, struct itimerspec *its);
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
		  struct itimerspec *ovalue);
int timer_getoverrun(timer_t timerid);
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
int clock_nanosleep(clockid_t clock_id, int flags,
		    const struct timespec *rqtp, struct timespec *rmtp);

#ifdef __cplusplus
}
#endif

#else /* ZEPHYR_INCLUDE_POSIX_TIME_H_ */
/* Read the toolchain header when <posix/time.h> finds itself on the
 * first attempt.
 */
#include_next <time.h>
#endif /* ZEPHYR_INCLUDE_POSIX_TIME_H_ */
