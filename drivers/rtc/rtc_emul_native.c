/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <stdint.h>

int rtc_emul_native_gettime(int64_t *sec, int64_t *nsec)
{
	struct timespec host_time;

	if (clock_gettime(CLOCK_REALTIME, &host_time) == 0) {
		*sec = host_time.tv_sec;
		*nsec = host_time.tv_nsec;
		return 0;
	} else {
		return -1;
	}
}
