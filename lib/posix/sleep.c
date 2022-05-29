/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
/* required for struct timespec */
#include <time.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include <zephyr/kernel.h>

/**
 * @brief Sleep for a specified number of seconds.
 *
 * See IEEE 1003.1
 */
unsigned sleep(unsigned int seconds)
{
	k_sleep(K_SECONDS(seconds));
	return 0;
}
/**
 * @brief Suspend execution for microsecond intervals.
 *
 * See IEEE 1003.1
 */
int usleep(useconds_t useconds)
{
	if (useconds < USEC_PER_MSEC) {
		k_busy_wait(useconds);
	} else {
		k_msleep(useconds / USEC_PER_MSEC);
	}

	return 0;
}

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
