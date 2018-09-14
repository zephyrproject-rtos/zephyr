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

struct timespec {
	signed int  tv_sec;
	signed int  tv_nsec;
};

struct itimerspec {
	struct timespec it_interval;  /* Timer interval */
	struct timespec it_value;     /* Timer expiration */
};

struct timeval {
	signed int  tv_sec;
	signed int  tv_usec;
};

#include <kernel.h>
#include <errno.h>
#include "sys/types.h"
#include "signal.h"

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
