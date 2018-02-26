/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include <pthread.h>

#define SLEEP_SECONDS 1

void main(void)
{
	u32_t status;
	s64_t nsecs_elapsed, secs_elapsed;
	struct timespec ts, te;

	TC_START("POSIX clock APIs\n");
	clock_gettime(CLOCK_MONOTONIC, &ts);
	/* 2 Sec Delay */
	sleep(SLEEP_SECONDS);
	usleep(SLEEP_SECONDS * USEC_PER_SEC);
	clock_gettime(CLOCK_MONOTONIC, &te);

	if (te.tv_nsec >= ts.tv_nsec) {
		secs_elapsed = te.tv_sec - ts.tv_sec;
		nsecs_elapsed = te.tv_nsec - ts.tv_nsec;
	} else {
		nsecs_elapsed = NSEC_PER_SEC + te.tv_nsec - ts.tv_nsec;
		secs_elapsed = (te.tv_sec - ts.tv_sec - 1);
	}

	status = secs_elapsed == (2 * SLEEP_SECONDS)  ? TC_PASS : TC_FAIL;

	TC_PRINT("POSIX clock APIs test done\n");
	TC_END_REPORT(status);
}
