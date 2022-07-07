/*
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <limits.h>
#include <errno.h>
/* required for struct timespec */
#include <zephyr/posix/time.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

/**
 * @brief Suspend execution for nanosecond intervals.
 *
 * See IEEE 1003.1
 */
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	uint64_t ns;
	uint64_t us;
	const bool update_rmtp = rmtp != NULL;

	if (rqtp == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (rqtp->tv_sec < 0 || rqtp->tv_nsec < 0
		|| rqtp->tv_nsec >= NSEC_PER_SEC) {
		errno = EINVAL;
		return -1;
	}

	if (rqtp->tv_sec == 0 && rqtp->tv_nsec == 0) {
		goto do_rmtp_update;
	}

	if (unlikely(rqtp->tv_sec >= ULLONG_MAX / NSEC_PER_SEC)) {
		/* If a user passes this in, we could be here a while, but
		 * at least it's technically correct-ish
		 */
		ns = rqtp->tv_nsec + NSEC_PER_SEC
			+ k_sleep(K_SECONDS(rqtp->tv_sec - 1)) * NSEC_PER_MSEC;
	} else {
		ns = rqtp->tv_sec * NSEC_PER_SEC + rqtp->tv_nsec;
	}

	/* TODO: improve upper bound when hr timers are available */
	us = ceiling_fraction(ns, NSEC_PER_USEC);
	do {
		us = k_usleep(us);
	} while (us != 0);

do_rmtp_update:
	if (update_rmtp) {
		rmtp->tv_sec = 0;
		rmtp->tv_nsec = 0;
	}

	return 0;
}
