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

#include <zephyr/sys/__assert.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline bool is_timespec_valid(const struct timespec *ts)
{
	__ASSERT_NO_MSG(ts != NULL);
	return (ts->tv_nsec >= 0) && (ts->tv_nsec < NSEC_PER_SEC);
}

static inline int64_t ts_to_ns(const struct timespec *ts)
{
	return ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec;
}

#define _tp_op(_a, _b, _op) (ts_to_ns(_a) _op ts_to_ns(_b))

#define _decl_op(_type, _name, _op)                                                                \
	static inline _type _name(const struct timespec *_a, const struct timespec *_b)            \
	{                                                                                          \
		return _tp_op(_a, _b, _op);                                                        \
	}

_decl_op(bool, tp_eq, ==);     /* a == b */
_decl_op(bool, tp_lt, <);      /* a < b */
_decl_op(bool, tp_gt, >);      /* a > b */
_decl_op(bool, tp_le, <=);     /* a <= b */
_decl_op(bool, tp_ge, >=);     /* a >= b */
_decl_op(int64_t, tp_diff, -); /* a - b */

/* lo <= (a - b) < hi */
static inline bool tp_diff_in_range_ns(const struct timespec *a, const struct timespec *b,
				       int64_t lo, int64_t hi)
{
	int64_t diff = tp_diff(a, b);

	return diff >= lo && diff < hi;
}

/**
 * @brief Convert an absolute time to a relative timeout in milliseconds.
 *
 * Convert the absolute time specified by @p abstime with respect to @p clock to a relative
 * timeout in milliseconds. The result is the number of milliseconds until the specified time,
 * clamped to the range [0, `INT32_MAX`], so that if @p abstime specifies a timepoint in the past,
 * then the resulting timeout is 0. If the clock specified by @p clock is not supported, the
 * function returns 0.
 *
 * @param clock The clock ID to use for conversion, e.g. `CLOCK_REALTIME`, or `CLOCK_MONOTONIC`.
 * @param abstime The absolute time to convert.
 * @return The relative timeout in milliseconds clamped to the range [0, `INT32_MAX`].
 */
uint32_t timespec_to_timeoutms(clockid_t clock, const struct timespec *abstime);

__syscall int __posix_clock_get_base(clockid_t clock_id, struct timespec *ts);

#include <zephyr/syscalls/posix_clock.h>

#ifdef __cplusplus
}
#endif

#endif
