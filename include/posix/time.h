/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_TIME_H_
#define ZEPHYR_INCLUDE_POSIX_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __timespec_defined
#define __timespec_defined
struct timespec {
	signed int tv_sec;
	signed int tv_nsec;
};
#endif

/* Older newlib's like 2.{0-2}.0 don't define any newlib version defines, only
 * __NEWLIB_H__ so we use that to decide if itimerspec was defined in
 * sys/types.h w/o any protection.  It appears sometime in the 2.3.0 version
 * of newlib did itimerspec move out of sys/types.h, however version 2.3.0
 * seems to report itself as __NEWLIB_MINOR__ == 2, where as 2.2.0 doesn't
 * even define __NEWLIB_MINOR__ or __NEWLIB__
 */
#if !defined(__NEWLIB_H__) || (__NEWLIB__ >= 3) || \
    (__NEWLIB__ == 2  && __NEWLIB_MINOR__ >= 2)
struct itimerspec {
	struct timespec it_interval;  /* Timer interval */
	struct timespec it_value;     /* Timer expiration */
};
#endif

struct timeval {
	signed int  tv_sec;
	signed int  tv_usec;
};

#include <kernel.h>
#include <errno.h>
#include "posix_types.h"
#include <posix/signal.h>

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

#define NSEC_PER_MSEC (NSEC_PER_USEC * USEC_PER_MSEC)

#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME 4
#endif

static inline s32_t _ts_to_ms(const struct timespec *to)
{
	return (to->tv_sec * MSEC_PER_SEC) + (to->tv_nsec / NSEC_PER_MSEC);
}

int clock_gettime(clockid_t clock_id, struct timespec *ts);
int clock_settime(clockid_t clock_id, const struct timespec *ts);
/* Timer APIs */
int timer_create(clockid_t clockId, struct sigevent *evp, timer_t *timerid);
int timer_delete(timer_t timerid);
int timer_gettime(timer_t timerid, struct itimerspec *its);
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
		  struct itimerspec *ovalue);

int gettimeofday(struct timeval *tv, const void *tz);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_TIME_H_ */
