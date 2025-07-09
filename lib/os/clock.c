/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/toolchain.h>

/*
 * `k_uptime_get` returns a timestamp offset on an always increasing
 * value from the system start.  To support the `SYS_CLOCK_REALTIME`
 * clock, this `rt_clock_offset` records the time that the system was
 * started.  This can either be set via 'sys_clock_settime', or could be
 * set from a real time clock, if such hardware is present.
 */
static struct timespec rt_clock_offset;
static struct k_spinlock rt_clock_offset_lock;

static bool is_valid_clock_id(int clock_id)
{
	switch (clock_id) {
	case SYS_CLOCK_MONOTONIC:
	case SYS_CLOCK_REALTIME:
		return true;
	default:
		return false;
	}
}

static void timespec_from_ticks(uint64_t ticks, struct timespec *ts)
{
	uint64_t elapsed_secs = ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	uint64_t nremainder = ticks % CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	*ts = (struct timespec){
		.tv_sec = (time_t)elapsed_secs,
		/* For ns 32 bit conversion can be used since its smaller than 1sec. */
		.tv_nsec = (int32_t)k_ticks_to_ns_floor32(nremainder),
	};
}

int sys_clock_from_clockid(int clock_id)
{
	switch (clock_id) {
#if defined(CLOCK_REALTIME) || defined(_POSIX_C_SOURCE)
	case (int)CLOCK_REALTIME:
		return SYS_CLOCK_REALTIME;
#endif
#if defined(CLOCK_MONOTONIC) || defined(_POSIX_MONOTONIC_CLOCK)
	case (int)CLOCK_MONOTONIC:
		return SYS_CLOCK_MONOTONIC;
#endif
	default:
		return -EINVAL;
	}
}

int sys_clock_gettime(int clock_id, struct timespec *ts)
{
	if (!is_valid_clock_id(clock_id)) {
		return -EINVAL;
	}

	switch (clock_id) {
	case SYS_CLOCK_REALTIME: {
		struct timespec offset;

		timespec_from_ticks(k_uptime_ticks(), ts);
		sys_clock_getrtoffset(&offset);
		if (unlikely(!timespec_add(ts, &offset))) {
			/* Saturate rather than reporting an overflow in 292 billion years */
			*ts = (struct timespec){
				.tv_sec = (time_t)INT64_MAX,
				.tv_nsec = NSEC_PER_SEC - 1,
			};
		}
	} break;

	case SYS_CLOCK_MONOTONIC:
		timespec_from_ticks(k_uptime_ticks(), ts);
		break;

	default:
		CODE_UNREACHABLE;
		return -EINVAL; /* Should never reach here */
	}

	__ASSERT_NO_MSG(timespec_is_valid(ts));

	return 0;
}

void z_impl_sys_clock_getrtoffset(struct timespec *tp)
{
	__ASSERT_NO_MSG(tp != NULL);

	K_SPINLOCK(&rt_clock_offset_lock) {
		*tp = rt_clock_offset;
	}

	__ASSERT_NO_MSG(timespec_is_valid(tp));
}

#ifdef CONFIG_USERSPACE
void z_vrfy_sys_clock_getrtoffset(struct timespec *tp)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(tp, sizeof(*tp)));
	return z_impl_sys_clock_getrtoffset(tp);
}
#include <zephyr/syscalls/sys_clock_getrtoffset_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_sys_clock_settime(int clock_id, const struct timespec *tp)
{
	struct timespec offset;

	if (clock_id != SYS_CLOCK_REALTIME) {
		return -EINVAL;
	}

	if (!timespec_is_valid(tp)) {
		return -EINVAL;
	}

	timespec_from_ticks(k_uptime_ticks(), &offset);
	(void)timespec_negate(&offset);
	(void)timespec_add(&offset, tp);

	K_SPINLOCK(&rt_clock_offset_lock) {
		rt_clock_offset = offset;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_sys_clock_settime(int clock_id, const struct timespec *ts)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(ts, sizeof(*ts)));
	return z_impl_sys_clock_settime(clock_id, ts);
}
#include <zephyr/syscalls/sys_clock_settime_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_sys_clock_nanosleep(int clock_id, int flags, const struct timespec *rqtp,
			       struct timespec *rmtp)
{
	k_timepoint_t end;
	k_timeout_t timeout;
	struct timespec duration;
	const bool update_rmtp = rmtp != NULL;
	const bool abstime = (flags & SYS_TIMER_ABSTIME) != 0;

	if (!is_valid_clock_id(clock_id)) {
		return -EINVAL;
	}

	if ((rqtp->tv_sec < 0) || !timespec_is_valid(rqtp)) {
		return -EINVAL;
	}

	if (abstime) {
		/* convert absolute time to relative time duration */
		(void)sys_clock_gettime(clock_id, &duration);
		(void)timespec_negate(&duration);
		(void)timespec_add(&duration, rqtp);
	} else {
		duration = *rqtp;
	}

	/* sleep for relative time duration */
	if ((sizeof(rqtp->tv_sec) == sizeof(int64_t)) &&
	    unlikely(rqtp->tv_sec >= (time_t)(UINT64_MAX / NSEC_PER_SEC))) {
		uint64_t ns = (uint64_t)k_sleep(K_SECONDS(duration.tv_sec - 1)) * NSEC_PER_MSEC;
		struct timespec rem = {
			.tv_sec = (time_t)(ns / NSEC_PER_SEC),
			.tv_nsec = ns % NSEC_PER_MSEC,
		};

		duration.tv_sec = 1;
		(void)timespec_add(&duration, &rem);
	}

	timeout = timespec_to_timeout(&duration, NULL);
	end = sys_timepoint_calc(timeout);
	do {
		(void)k_sleep(timeout);
		timeout = sys_timepoint_timeout(end);
	} while (!K_TIMEOUT_EQ(timeout, K_NO_WAIT));

	if (update_rmtp) {
		*rmtp = (struct timespec){
			.tv_sec = 0,
			.tv_nsec = 0,
		};
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_sys_clock_nanosleep(int clock_id, int flags, const struct timespec *rqtp,
			       struct timespec *rmtp)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(rqtp, sizeof(*rqtp)));
	if (rmtp != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(rmtp, sizeof(*rmtp)));
	}
	return z_impl_sys_clock_nanosleep(clock_id, flags, rqtp, rmtp);
}
#include <zephyr/syscalls/sys_clock_nanosleep_mrsh.c>
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
static void reset_clock_offset(void)
{
	K_SPINLOCK(&rt_clock_offset_lock) {
		rt_clock_offset = (struct timespec){0};
	}
}

static void clock_offset_reset_rule_after(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	reset_clock_offset();
}

ZTEST_RULE(clock_offset_reset_rule, NULL, clock_offset_reset_rule_after);
#endif /* CONFIG_ZTEST */
