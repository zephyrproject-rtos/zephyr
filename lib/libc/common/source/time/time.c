/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

/* clock_gettime() prototype */
#include <zephyr/posix/time.h>

time_t time(time_t *tloc)
{
	struct timespec ts;
	int ret;

	ret = clock_gettime(CLOCK_REALTIME, &ts);
	if (ret < 0) {
		/* errno is already set by clock_gettime */
		return (time_t) -1;
	}

	if (tloc) {
		*tloc = ts.tv_sec;
	}

	return ts.tv_sec;
}
