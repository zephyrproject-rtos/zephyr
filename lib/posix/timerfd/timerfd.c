/*
 * Copyright (c) 2025 Atym, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../options/posix_clock.h"
#include <string.h>

#include <zephyr/posix/sys/timerfd.h>
#include <zephyr/zvfs/timerfd.h>

int timerfd_create(int clockid, int flags)
{
    return zvfs_timerfd(clockid, flags);
};

int timerfd_gettime(int fd, struct itimerspec *curr_value)
{
    int ret;
    uint32_t value, period;
   	int32_t leftover;
	int64_t nsecs, secs;

    ret = zvfs_timerfd_gettime(fd, &value, &period);
    if (ret != 0) {
        return ret;
    }

    /* converts value */
    secs =  value / MSEC_PER_SEC;
	leftover = value - (secs * MSEC_PER_SEC);
	nsecs = (int64_t)leftover * NSEC_PER_MSEC;
	curr_value->it_value.tv_sec = (int32_t) secs;
	curr_value->it_value.tv_nsec = (int32_t) nsecs;

	/* converts period */
	secs =  period / MSEC_PER_SEC;
    leftover = period - (secs * MSEC_PER_SEC);
    nsecs = (int64_t)leftover * NSEC_PER_MSEC;
    curr_value->it_interval.tv_sec = (int32_t) secs;
    curr_value->it_interval.tv_nsec = (int32_t) nsecs;

    return 0;
};

int timerfd_settime(int fd, int flags, const struct	itimerspec *new_value,
	   struct itimerspec *old_value)
{
    uint32_t period, duration, current;

   	if (!timespec_is_valid(&new_value->it_interval) ||
	    !timespec_is_valid(&new_value->it_value)) {
		errno = EINVAL;
		return -1;
	}

   	/*  Save time to expire and old reload value. */
	if (old_value != NULL) {
		timerfd_gettime(fd, old_value);
	}

	/* Calculate timer duration */
	duration = ts_to_ms(&(new_value->it_value));

	/* Calculate timer period */
	period = ts_to_ms(&(new_value->it_interval));

	if ((flags & TFD_TIMER_ABSTIME) != 0) {
    	zvfs_timerfd_gettime(fd, &current, NULL);

		if (current >= duration) {
			duration = 0U;
		} else {
			duration -= current;
		}
	}

    return zvfs_timerfd_settime(fd, K_MSEC(duration), K_MSEC(period));
}
