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
#include <zephyr/sys/clock.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/internal/syscall_handler.h>

/*
 * `k_uptime_get` returns a timestamp based on an always increasing
 * value from the system start.  To support the `SYS_CLOCK_REALTIME`
 * clock, this `rt_clock_base` records the time that the system was
 * started.  This can either be set via 'sys_clock_settime', or could be
 * set from a real time clock, if such hardware is present.
 */
static struct timespec rt_clock_base;
static struct k_spinlock rt_clock_base_lock;

static inline bool is_valid_clock_id(int clock_id)
{
	switch (clock_id) {
	case SYS_CLOCK_MONOTONIC:
	case SYS_CLOCK_REALTIME:
		return true;
	default:
		return false;
	}
}

static inline void timespec_from_ticks(uint64_t ticks, struct timespec *ts)
{
	uint64_t elapsed_secs = ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	uint64_t nremainder = ticks - elapsed_secs * CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	ts->tv_sec = (time_t)elapsed_secs;
	/* For ns 32 bit conversion can be used since its smaller than 1sec. */
	ts->tv_nsec = (int32_t)k_ticks_to_ns_floor32(nremainder);
}

int z_impl_sys_clock_gettime(int clock_id, struct timespec *ts)
{
	struct timespec base;

	if (!is_valid_clock_id(clock_id)) {
		return -EINVAL;
	}

	if (clock_id == SYS_CLOCK_REALTIME) {
		K_SPINLOCK(&rt_clock_base_lock) {
			base = rt_clock_base;
		}
	} else {
		base = (struct timespec){0};
	}

	timespec_from_ticks(k_uptime_ticks(), ts);
	if (!timespec_add(ts, &base)) {
		return -EOVERFLOW;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_sys_clock_gettime(int clock_id, struct timespec *ts)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(ts, sizeof(*ts)));
	return z_impl_sys_clock_gettime(clock_id, ts);
}
#include <zephyr/syscalls/sys_clock_gettime_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_sys_clock_settime(int clock_id, const struct timespec *tp)
{
	struct timespec base;

	if (clock_id != SYS_CLOCK_REALTIME) {
		return -EINVAL;
	}

	if (!timespec_is_valid(tp)) {
		return -EINVAL;
	}

	uint64_t elapsed_nsecs = k_ticks_to_ns_floor64(k_uptime_ticks());
	int64_t delta = (int64_t)NSEC_PER_SEC * tp->tv_sec + tp->tv_nsec - elapsed_nsecs;

	base.tv_sec = delta / NSEC_PER_SEC;
	base.tv_nsec = delta % NSEC_PER_SEC;

	K_SPINLOCK(&rt_clock_base_lock) {
		rt_clock_base = base;
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
	uint64_t ns;
	uint64_t uptime_ns;
	const bool update_rmtp = rmtp != NULL;
	const bool abstime = (flags & SYS_TIMER_ABSTIME) != 0;

	if (!is_valid_clock_id(clock_id)) {
		return -EINVAL;
	}

	if ((rqtp->tv_sec < 0) || !timespec_is_valid(rqtp)) {
		return -EINVAL;
	}

	if (!abstime && unlikely(rqtp->tv_sec >= ULLONG_MAX / NSEC_PER_SEC)) {
		ns = rqtp->tv_nsec + NSEC_PER_SEC +
		     (uint64_t)k_sleep(K_SECONDS(rqtp->tv_sec - 1)) * NSEC_PER_MSEC;
	} else {
		ns = (uint64_t)rqtp->tv_sec * NSEC_PER_SEC + rqtp->tv_nsec;
	}

	uptime_ns = k_ticks_to_ns_ceil64(sys_clock_tick_get());

	if (abstime && (clock_id == SYS_CLOCK_REALTIME)) {
		K_SPINLOCK(&rt_clock_base_lock) {
			ns -= rt_clock_base.tv_sec * NSEC_PER_SEC + rt_clock_base.tv_nsec;
		}
	}

	if (!abstime) {
		ns += uptime_ns;
	}

	if (ns <= uptime_ns) {
		goto do_rmtp_update;
	}

	do {
		ns = k_sleep(K_TIMEOUT_ABS_NS(ns)) * NSEC_PER_MSEC;
	} while (ns != 0);

do_rmtp_update:
	if (update_rmtp) {
		rmtp->tv_sec = 0;
		rmtp->tv_nsec = 0;
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
static void reset_clock_base(void)
{
	K_SPINLOCK(&rt_clock_base_lock) {
		rt_clock_base = (struct timespec){0};
	}
}

static void clock_base_reset_rule_after(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	reset_clock_base();
}

ZTEST_RULE(clock_base_reset_rule, NULL, clock_base_reset_rule_after);
#endif /* CONFIG_ZTEST */
