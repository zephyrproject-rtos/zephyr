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
#include <sys_clock.h>

/**
 * @brief Suspend execution for nanosecond intervals.
 *
 * See IEEE 1003.1
 */
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	uint64_t ns;
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
		ns = rqtp->tv_nsec + NSEC_PER_SEC;
		k_sleep(K_SECONDS(rqtp->tv_sec - 1));
	} else {
		ns = rqtp->tv_sec * NSEC_PER_SEC + rqtp->tv_nsec;
	}

	/* currently we have no mechanism to achieve greater resolution */
	k_busy_wait(ns / NSEC_PER_USEC);

do_rmtp_update:
	if (update_rmtp) {
		rmtp->tv_sec = 0;
		rmtp->tv_nsec = 0;
	}

	return 0;
}
