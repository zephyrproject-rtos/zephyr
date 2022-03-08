/*
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <kernel.h>
#include <limits.h>
#include <errno.h>
/* required for struct timespec */
#include <posix/time.h>
#include <sys/util.h>
#include <sys_clock.h>

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

	if ((rqtp->tv_sec < 0) || (rqtp->tv_nsec < 0)
		|| (rqtp->tv_nsec >= (long)NSEC_PER_SEC)) {
		errno = EINVAL;
		return -1;
	}

	if ((rqtp->tv_sec == 0) && (rqtp->tv_nsec == 0)) {
		goto do_rmtp_update;
	}

	if (unlikely((unsigned long long)rqtp->tv_sec >= (ULLONG_MAX / NSEC_PER_SEC))) {
		/* If a user passes this in, we could be here a while, but
		 * at least it's technically correct-ish
		 */
		ns = (uint64_t)rqtp->tv_nsec + NSEC_PER_SEC
			+ ((uint64_t)k_sleep(K_SECONDS(((uint64_t)rqtp->tv_sec - 1)))
				* NSEC_PER_MSEC);
	} else {
		ns = ((uint64_t)rqtp->tv_sec * NSEC_PER_SEC) + (uint64_t)rqtp->tv_nsec;
	}

	/* TODO: improve upper bound when hr timers are available */
	us = ceiling_fraction(ns, NSEC_PER_USEC);
	/*? Possible overflow? Why k_usleep takes a signed integer? */
	int32_t remaining = (int32_t)us;
	do {
		remaining = k_usleep(remaining);
	} while (remaining != 0);

do_rmtp_update:
	if (update_rmtp) {
		rmtp->tv_sec = 0;
		rmtp->tv_nsec = 0;
	}

	return 0;
}
