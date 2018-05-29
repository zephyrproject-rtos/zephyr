/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <pthread.h>

#define N_THR 3
#define STACKSZ 1024
#define ONE_SECOND 1
#define THREAD_PRIORITY 2

K_THREAD_STACK_ARRAY_DEFINE(stacks, N_THR, STACKSZ);
int exit_count;

void *thread_top(void *p1)
{
	pthread_t pthread;
	u32_t policy, seconds;
	struct sched_param param;

	pthread = (pthread_t) pthread_self();
	pthread_getschedparam(pthread, &policy, &param);
	printk("Thread %d scheduling policy = %d & priority %d started\n",
	       (s32_t) p1, policy, param.priority);
	seconds = (s32_t)p1;
	sleep(seconds * ONE_SECOND);
	exit_count++;
	printk("Exiting thread %d\n", (s32_t)p1);
	pthread_exit(p1);
	return NULL;
}

static bool is_sched_prio_valid(int prio, int policy)
{
	if (prio < sched_get_priority_min(policy) ||
	    prio > sched_get_priority_max(policy)) {
		return false;
	}

	return true;
}

void test_pthread_join(void)
{
	s32_t i, ret;
	pthread_attr_t attr[N_THR];
	struct sched_param schedparam;
	pthread_t newthread[N_THR];
	u32_t detachstate, schedpolicy = SCHED_RR;
	void *retval;
	size_t stack_size;

	printk("POSIX pthread join API\n");
	/* Creating threads with lowest application priority */
	for (i = 0; i < N_THR; i++) {
		ret = pthread_attr_init(&attr[i]);
		if (ret != 0) {
			zassert_false(pthread_attr_destroy(&attr[i]),
				      "Unable to destroy pthread object attrib");
			zassert_false(pthread_attr_init(&attr[i]),
				      "Unable to create pthread object attrib");
		}

		/* Setting pthread as JOINABLE */
		pthread_attr_getdetachstate(&attr[i], &detachstate);
		if (detachstate != PTHREAD_CREATE_JOINABLE) {
			pthread_attr_setdetachstate(&attr[i],
						    PTHREAD_CREATE_JOINABLE);
		}

		/* Setting premptive scheduling policy */
		pthread_attr_getschedpolicy(&attr[i], &schedpolicy);
		if (schedpolicy != SCHED_RR) {
			schedpolicy = SCHED_RR;
			pthread_attr_setschedpolicy(&attr[i], schedpolicy);
		}

		/* Setting scheduling priority */
		pthread_attr_getschedparam(&attr[i], &schedparam);
		if (schedparam.priority != THREAD_PRIORITY) {
			schedparam.priority = THREAD_PRIORITY;

			ret = is_sched_prio_valid(schedparam.priority,
					schedpolicy);

			/*TESTPOINT: Check if priority is valid*/
			zassert_true(ret, "Scheduling priority invalid");

			pthread_attr_setschedparam(&attr[i], &schedparam);
		}

		/* Setting stack */
		pthread_attr_getstacksize(&attr[i], &stack_size);
		if (stack_size != STACKSZ) {
			stack_size = STACKSZ;
			pthread_attr_setstack(&attr[i], &stacks[i][0],
					      stack_size);
		}

		ret = pthread_create(&newthread[i], &attr[i], thread_top,
				     (void *)i);

		/*TESTPOINT: Check if thread created successfully*/
		zassert_false(ret, "Number of threads exceed max limit");

		pthread_attr_destroy(&attr[i]);
	}

	for (i = 0; i < N_THR; i++) {
		printk("Waiting for pthread %d to join\n", i);
		pthread_join(newthread[i], &retval);
		printk("Pthread %d joined to %s\n", i, __func__);
	}

	/* Test PASS if all threads have exited before main exit */
	zassert_equal(exit_count, N_THR, "pthread join test failed");
}

void test_main(void)
{
	ztest_test_suite(test_pthreads_join,
			ztest_unit_test(test_pthread_join));
	ztest_run_test_suite(test_pthreads_join);
}
