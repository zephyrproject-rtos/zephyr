/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __POSIX_TIME_H__
#define __POSIX_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_NEWLIB_LIBC
#include_next <time.h>
#else
struct timespec {
	s32_t tv_sec;
	s32_t tv_nsec;
};
#endif /* CONFIG_NEWLIB_LIBC */
#include <kernel.h>
#include <errno.h>
#include "sys/types.h"

#define CLOCK_MONOTONIC 1

static inline s32_t _ts_to_ms(const struct timespec *to)
{
	return (to->tv_sec * 1000) + (to->tv_nsec / 1000000);
}

/**
 * @brief Set clock time.
 *
 * See IEEE 1003.1
 */
static inline int clock_settime(clockid_t clock_id, const struct timespec *ts)
{
	errno = ENOSYS;
	return -1;
}

int clock_gettime(clockid_t clock_id, struct timespec *ts);

#ifdef __cplusplus
}
#endif

#endif /* __POSIX_TIME_H__ */
