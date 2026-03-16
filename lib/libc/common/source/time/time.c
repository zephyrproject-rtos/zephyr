/*
 * Copyright (c) 2021 Golioth, Inc.
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

#include <zephyr/sys/clock.h>

time_t time(time_t *tloc)
{
	struct timespec ts;
	int ret;

	ret = sys_clock_gettime(SYS_CLOCK_REALTIME, &ts);
	if (ret < 0) {
		return (time_t) -1;
	}

	if (tloc) {
		*tloc = ts.tv_sec;
	}

	return ts.tv_sec;
}
