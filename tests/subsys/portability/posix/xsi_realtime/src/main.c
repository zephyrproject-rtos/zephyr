/*
 * Copyright (c) 2025 Marvin Ouma <pancakesdeath@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/ztest.h>

ZTEST(xsi_realtime, test_sched_getparam)
{
	struct sched_param param;
	int rc = sched_getparam(0, &param);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}

ZTEST(xsi_realtime, test_sched_getscheduler)
{
	int rc = sched_getscheduler(0);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}
ZTEST(xsi_realtime, test_sched_setparam)
{
	struct sched_param param = {
		.sched_priority = 2,
	};
	int rc = sched_setparam(0, &param);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}

ZTEST(xsi_realtime, test_sched_setscheduler)
{
	struct sched_param param = {
		.sched_priority = 2,
	};
	int policy = 0;
	int rc = sched_setscheduler(0, policy, &param);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}

ZTEST(xsi_realtime, test_sched_rr_get_interval)
{
	struct timespec interval = {
		.tv_sec = 0,
		.tv_nsec = 0,
	};
	int rc = sched_rr_get_interval(0, &interval);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}

ZTEST_SUITE(xsi_realtime, NULL, NULL, NULL, NULL, NULL);
