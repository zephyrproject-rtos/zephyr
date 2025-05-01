/*
 * Copyright (c) 2023, Meta
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_POSIX_CLOCK_H_
#define ZEPHYR_LIB_POSIX_POSIX_CLOCK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <zephyr/sys_clock.h>
#include <zephyr/sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

static inline bool timespec_is_valid(const struct timespec *ts)
{
	__ASSERT_NO_MSG(ts != NULL);
	return (ts->tv_nsec >= 0) && (ts->tv_nsec < NSEC_PER_SEC);
}

static inline int64_t ts_to_ns(const struct timespec *ts)
{
	return ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec;
}

static inline int64_t ts_to_ms(const struct timespec *ts)
{
	return ts->tv_sec * MSEC_PER_SEC + ts->tv_nsec / NSEC_PER_MSEC;
}

static inline bool tp_ge(const struct timespec *a, const struct timespec *b)
{
	return ts_to_ns(a) >= ts_to_ns(b);
}

static inline int64_t tp_diff(const struct timespec *a, const struct timespec *b)
{
	return ts_to_ns(a) - ts_to_ns(b);
}

/* lo <= (a - b) < hi */
static inline bool tp_diff_in_range_ns(const struct timespec *a, const struct timespec *b,
				       int64_t lo, int64_t hi)
{
	int64_t diff = tp_diff(a, b);

	return diff >= lo && diff < hi;
}

uint32_t timespec_to_timeoutms(clockid_t clock_id, const struct timespec *abstime);

__syscall int __posix_clock_get_base(clockid_t clock_id, struct timespec *ts);

/** INTERNAL_HIDDEN @endcond */

#include <zephyr/syscalls/posix_clock.h>

#ifdef __cplusplus
}
#endif

#endif
