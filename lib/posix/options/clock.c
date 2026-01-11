/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/sys/clock.h>

int clock_gettime(clockid_t clock_id, struct timespec *ts)
{
	int ret;

	ret = sys_clock_gettime(sys_clock_from_clockid((int)clock_id), ts);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

int clock_getres(clockid_t clock_id, struct timespec *res)
{
	BUILD_ASSERT(CONFIG_SYS_CLOCK_TICKS_PER_SEC > 0 &&
			     CONFIG_SYS_CLOCK_TICKS_PER_SEC <= NSEC_PER_SEC,
		     "CONFIG_SYS_CLOCK_TICKS_PER_SEC must be > 0 and <= NSEC_PER_SEC");

	if (!(clock_id == CLOCK_MONOTONIC || clock_id == CLOCK_REALTIME ||
	      clock_id == CLOCK_PROCESS_CPUTIME_ID)) {
		errno = EINVAL;
		return -1;
	}

	if (res != NULL) {
		*res = (struct timespec){
			.tv_sec = 0,
			.tv_nsec = NSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC,
		};
	}

	return 0;
}

/**
 * @brief Set the time of the specified clock.
 *
 * See IEEE 1003.1.
 *
 * Note that only the `CLOCK_REALTIME` clock can be set using this
 * call.
 */
int clock_settime(clockid_t clock_id, const struct timespec *tp)
{
	int ret;

	ret = sys_clock_settime(sys_clock_from_clockid((int)clock_id), tp);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

/*
 * Note: usleep() was removed in Issue 7.
 *
 * It is kept here for compatibility purposes.
 *
 * For more information, please see
 * https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_xsh_chap01.html
 * https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_xsh_chap03.html
 */
int usleep(useconds_t useconds)
{
	int32_t rem;

	if (useconds >= USEC_PER_SEC) {
		errno = EINVAL;
		return -1;
	}

	rem = k_usleep(useconds);
	__ASSERT_NO_MSG(rem >= 0);
	if (rem > 0) {
		/* sleep was interrupted by a call to k_wakeup() */
		errno = EINTR;
		return -1;
	}

	return 0;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	int ret;

	if (rqtp == NULL) {
		errno = EFAULT;
		return -1;
	}

	ret = sys_clock_nanosleep(SYS_CLOCK_REALTIME, 0, rqtp, rmtp);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

int clock_getcpuclockid(pid_t pid, clockid_t *clock_id)
{
	/* We don't allow any process ID but our own.  */
	if (pid != 0 && pid != getpid()) {
		return EPERM;
	}

	*clock_id = CLOCK_PROCESS_CPUTIME_ID;

	return 0;
}
