/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <pthread.h>
#include <time.h>

#define DURATION_SECS 1
#define DURATION_NSECS 0
#define PERIOD_SECS 0
#define PERIOD_NSECS 100000000

static int exp_count;

enum sched_policy_type {
	SCHED_IDLE = 2,
	SCHED_DEADLINE = 3,
	SCHED_OTHER = 4,
	INVAL = 10,
};

static const enum sched_policy_type spt_map[] = {
	SCHED_IDLE,
	SCHED_DEADLINE,
	SCHED_OTHER,
	INVAL,
};

void check_sched_policy(int policy)
{
	int min_prio, max_prio;

	min_prio = sched_get_priority_min(policy);
	max_prio = sched_get_priority_max(policy);

	if (min_prio != -1 && max_prio != -1) {
		printk("The given policy number %d is supported. ",
				policy);
		printk("The minimum priority is %d and ", min_prio);
		printk("the maximum priority is %d.\n", max_prio);
	} else {
		printk("The given policy number %d is not supported.\n",
				policy);
	}
}

/*
 * Test which scheduling policies are supported and return
 * their maximum and minimum priorities if supported
 */

void test_sched_policy(void)
{
	int i;
	int schedpolicy = SCHED_FIFO;

	printk("\n");
	check_sched_policy(schedpolicy);
	schedpolicy = SCHED_RR;
	check_sched_policy(schedpolicy);
	for (i = 0; spt_map[i] != INVAL; i++) {
		check_sched_policy(spt_map[i]);
	}
	printk("\n");
}

void check_clock_support(int clock_id)
{
	int ret;
	struct timespec t;

	ret = clock_gettime(clock_id, &t);
	if (ret != -1) {
		printk("The given clock ID %d is supported.\n",
				clock_id);
	} else {
		printk("The given clock ID %d is not supported.\n",
				clock_id);
	}
}

/* Test which clocks are supported */

void test_clock_support(void)
{
	printk("\n");
	check_clock_support(CLOCK_MONOTONIC);
	check_clock_support(CLOCK_REALTIME);
	printk("\n");
}

void handler(union sigval val)
{
	printk("Handler signal value: %d for %d times\n",
			val.sival_int, ++exp_count);
}

/*
 * Verify timer APIs work as expected in
 * both positive and negative scenarios
 */

void check_timer_support(int clock_id)
{
	int ret;
	struct sigevent sig = {0};
	timer_t timerid;
	struct itimerspec value, ovalue;

	sig.sigev_notify = SIGEV_SIGNAL;
	sig.sigev_notify_function = handler;
	sig.sigev_value.sival_int = 20;

	ret = timer_create(clock_id, &sig, &timerid);
	if (ret != 0) {
		printk("Timer with clock ID %d is not supported.\n",
				clock_id);
		timerid = 0;
	} else {
		printk("Timer with clock ID %d created with timer ID %lu\n",
				clock_id, timerid);
		value.it_value.tv_sec = DURATION_SECS;
		value.it_value.tv_nsec = DURATION_NSECS;
		value.it_interval.tv_sec = PERIOD_SECS;
		value.it_interval.tv_nsec = PERIOD_NSECS;
	}

	timer_settime(timerid, 0, &value, &ovalue);
	usleep(100 * USEC_PER_MSEC);
	ret = timer_gettime(timerid, &value);
	if (ret != 0 && timerid == 0) {
		printk("Timer set fails with unsupported clock\n");
	} else if (ret == 0) {
		printk("Timer fires every %d secs and %d nsecs\n",
				(int) value.it_interval.tv_sec,
				(int) value.it_interval.tv_nsec);
	}

	ret = timer_delete(timerid);
	if (ret != 0) {
		printk("No timer to delete!\n");
	} else {
		printk("Timer deleted successfully.\n\n");
	}
}

/*
 * Test timer APIs with clocks which are both
 * supported and unsupported
 */

void test_timer_support(void)
{
	printk("\n");
	check_timer_support(CLOCK_MONOTONIC);
	check_timer_support(CLOCK_REALTIME);
	printk("\n");
}

void test_main(void)
{
	ztest_test_suite(test_posix_checks,
			ztest_unit_test(test_sched_policy),
			ztest_unit_test(test_clock_support),
			ztest_unit_test(test_timer_support));
	ztest_run_test_suite(test_posix_checks);
}
