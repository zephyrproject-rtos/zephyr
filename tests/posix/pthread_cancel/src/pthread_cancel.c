/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <pthread.h>

int pthread_cancel(pthread_t pthread);
#define N_THR 4
#define STACKSZ 1024
#define ONE_SECOND 1
#define THREAD_PRIORITY 3

K_THREAD_STACK_ARRAY_DEFINE(stacks, N_THR, STACKSZ);
int exit_count;

void *thread_top(void *p1)
{
	pthread_t self;
	int oldstate;
	int val = (u32_t) p1;
	struct sched_param param;

	param.priority = N_THR - (s32_t) p1;

	self = pthread_self();
	/* Change priority of thread */
	zassert_false(pthread_setschedparam(self, SCHED_RR, &param),
		      "Unable to set thread priority");

	if (val % 2) {
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	}

	if (val >= 2) {
		pthread_detach(self);
	}

	printk("Cancelling thread %d\n", (s32_t) p1);
	pthread_cancel(self);
	printk("Thread %d could not be cancelled\n", (s32_t) p1);
	sleep(ONE_SECOND);
	exit_count++;
	pthread_exit(p1);
	printk("Exited thread %d\n", (s32_t)p1);
	return NULL;
}

void test_pthread_cancel(void)
{
	s32_t i, ret;
	pthread_attr_t attr[N_THR];
	struct sched_param schedparam;
	pthread_t newthread[N_THR];
	void *retval;

	printk("POSIX thread cancel APIs\n");
	/* Creating 4 threads with lowest application priority */
	for (i = 0; i < N_THR; i++) {
		ret = pthread_attr_init(&attr[i]);
		if (ret != 0) {
			zassert_false(pthread_attr_destroy(&attr[i]),
				      "Unable to destroy pthread object attrib");
			zassert_false(pthread_attr_init(&attr[i]),
				      "Unable to create pthread object attrib");
		}

		if (i == 1) {
			pthread_attr_setdetachstate(&attr[i],
						    PTHREAD_CREATE_JOINABLE);
		}  else if (i == 2) {
			pthread_attr_setdetachstate(&attr[i],
						    PTHREAD_CREATE_DETACHED);
		}

		schedparam.priority = 2;
		pthread_attr_setschedparam(&attr[i], &schedparam);
		pthread_attr_setstack(&attr[i], &stacks[i][0], STACKSZ);
		ret = pthread_create(&newthread[i], &attr[i], thread_top,
				     (void *)i);

		zassert_false(ret, "Not enough space to create new thread");
	}

	for (i = 0; i < N_THR; i++) {
		printk("Waiting for pthread %d to Join\n", i);
		pthread_join(newthread[i], &retval);
		printk("Pthread %d joined to %s\n", i, __func__);
	}

	printk("Pthread join test over %d\n", exit_count);

	/* Test PASS if all threads have exited before main exit */
	zassert_equal(exit_count, 1, "pthread_cancel test failed");
}

void test_main(void)
{
	ztest_test_suite(test_pthreads_cancel,
			ztest_unit_test(test_pthread_cancel));
	ztest_run_test_suite(test_pthreads_cancel);
}
