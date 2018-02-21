/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <tc_util.h>

#define SECS_TO_SLEEP 2
#define DURATION_SECS 1
#define DURATION_NSECS 0
#define PERIOD_SECS 0
#define PERIOD_NSECS 100000000

static int exp_count;

void handler(union sigval val)
{
	TC_PRINT("Handler Signal value :%d for %d times\n", val.sival_int,
		 ++exp_count);
}

void main(void)
{
	int ret, status = TC_FAIL;
	struct sigevent sig;
	timer_t timerid;
	struct itimerspec value, ovalue;
	struct timespec ts, te;
	s64_t nsecs_elapsed, secs_elapsed, total_secs_timer;

	sig.sigev_notify = SIGEV_SIGNAL;
	sig.sigev_notify_function = handler;
	sig.sigev_value.sival_int = 20;
	sig.sigev_notify_attributes = NULL;
	TC_START("POSIX timer test\n");
	ret = timer_create(CLOCK_MONOTONIC, &sig, &timerid);

	if (ret == 0) {
		value.it_value.tv_sec = DURATION_SECS;
		value.it_value.tv_nsec = DURATION_NSECS;
		value.it_interval.tv_sec = PERIOD_SECS;
		value.it_interval.tv_nsec = PERIOD_NSECS;
		ret = timer_settime(timerid, 0, &value, &ovalue);
		clock_gettime(CLOCK_MONOTONIC, &ts);

		if (ret == 0) {
			sleep(SECS_TO_SLEEP);
		} else {
			TC_PRINT("posix timer failed to start, error %d\n",
				 errno);
			goto test_end;
		}

		clock_gettime(CLOCK_MONOTONIC, &te);
		timer_delete(timerid);
	} else {
		TC_ERROR("POSIX timer create failed with %d\n", errno);
		goto test_end;
	}

	if (te.tv_nsec >= ts.tv_nsec) {
		secs_elapsed = te.tv_sec - ts.tv_sec;
		nsecs_elapsed = te.tv_nsec - ts.tv_nsec;
	} else {
		nsecs_elapsed = NSEC_PER_SEC + te.tv_nsec - ts.tv_nsec;
		secs_elapsed = (te.tv_sec - ts.tv_sec - 1);
	}

	total_secs_timer = (u64_t) (value.it_value.tv_sec * NSEC_PER_SEC +
				    value.it_value.tv_nsec + exp_count *
				    (value.it_interval.tv_sec * NSEC_PER_SEC +
				     value.it_interval.tv_nsec)) / NSEC_PER_SEC;

	status = (total_secs_timer == secs_elapsed) ? TC_PASS : TC_FAIL;

test_end:
	TC_END_REPORT(status);
}
