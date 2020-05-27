/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <pthread.h>
#include <sys/util.h>

#define N_THR 3
#define STACKSZ (1024 + CONFIG_TEST_EXTRA_STACKSIZE)

K_THREAD_STACK_ARRAY_DEFINE(stack, N_THR, STACKSZ);
pthread_rwlock_t rwlock;

static void *thread_top(void *p1)
{
	pthread_t pthread;
	uint32_t policy, ret = 0U;
	struct sched_param param;
	int id = POINTER_TO_INT(p1);

	pthread = (pthread_t) pthread_self();
	pthread_getschedparam(pthread, &policy, &param);
	printk("Thread %d scheduling policy = %d & priority %d started\n",
	       id, policy, param.sched_priority);

	ret = pthread_rwlock_tryrdlock(&rwlock);
	if (ret) {
		printk("Not able to get RD lock on trying, try again\n");
		zassert_false(pthread_rwlock_rdlock(&rwlock),
			      "Failed to acquire write lock");
	}

	printk("Thread %d got RD lock\n", id);
	usleep(USEC_PER_MSEC);
	printk("Thread %d releasing RD lock\n", id);
	zassert_false(pthread_rwlock_unlock(&rwlock), "Failed to unlock");

	printk("Thread %d acquiring WR lock\n", id);
	ret = pthread_rwlock_trywrlock(&rwlock);
	if (ret != 0U) {
		zassert_false(pthread_rwlock_wrlock(&rwlock),
			      "Failed to acquire WR lock");
	}

	printk("Thread %d acquired WR lock\n", id);
	usleep(USEC_PER_MSEC);
	printk("Thread %d releasing WR lock\n", id);
	zassert_false(pthread_rwlock_unlock(&rwlock), "Failed to unlock");
	pthread_exit(NULL);
	return NULL;
}

void test_posix_rw_lock(void)
{
	int32_t i, ret;
	pthread_attr_t attr[N_THR];
	struct sched_param schedparam;
	pthread_t newthread[N_THR];
	struct timespec time;
	void *status;

	time.tv_sec = 1;
	time.tv_nsec = 0;

	zassert_equal(pthread_rwlock_destroy(&rwlock), EINVAL, NULL);
	zassert_equal(pthread_rwlock_rdlock(&rwlock), EINVAL, NULL);
	zassert_equal(pthread_rwlock_wrlock(&rwlock), EINVAL, NULL);
	zassert_equal(pthread_rwlock_trywrlock(&rwlock), EINVAL, NULL);
	zassert_equal(pthread_rwlock_tryrdlock(&rwlock), EINVAL, NULL);
	zassert_equal(pthread_rwlock_timedwrlock(&rwlock, &time), EINVAL, NULL);
	zassert_equal(pthread_rwlock_timedrdlock(&rwlock, &time), EINVAL, NULL);
	zassert_equal(pthread_rwlock_unlock(&rwlock), EINVAL, NULL);

	zassert_false(pthread_rwlock_init(&rwlock, NULL),
		      "Failed to create rwlock");
	printk("\nmain acquire WR lock and 3 threads acquire RD lock\n");
	zassert_false(pthread_rwlock_timedwrlock(&rwlock, &time),
		      "Failed to acquire write lock");

	/* Creating N premptive threads in increasing order of priority */
	for (i = 0; i < N_THR; i++) {
		zassert_equal(pthread_attr_init(&attr[i]), 0,
			      "Unable to create pthread object attrib");

		/* Setting scheduling priority */
		schedparam.sched_priority = i + 1;
		pthread_attr_setschedparam(&attr[i], &schedparam);

		/* Setting stack */
		pthread_attr_setstack(&attr[i], &stack[i][0], STACKSZ);

		ret = pthread_create(&newthread[i], &attr[i], thread_top,
				     INT_TO_POINTER(i));
		zassert_false(ret, "Low memory to thread new thread");

	}

	/* Delay to give change to child threads to run */
	usleep(USEC_PER_MSEC);
	printk("Parent thread releasing WR lock\n");
	zassert_false(pthread_rwlock_unlock(&rwlock), "Failed to unlock");

	/* Let child threads acquire RD Lock */
	usleep(USEC_PER_MSEC);
	printk("Parent thread acquiring WR lock again\n");

	time.tv_sec = 2;
	time.tv_nsec = 0;
	ret = pthread_rwlock_timedwrlock(&rwlock, &time);

	if (ret) {
		zassert_false(pthread_rwlock_wrlock(&rwlock),
			      "Failed to acquire write lock");
	}

	printk("Parent thread acquired WR lock again\n");
	usleep(USEC_PER_MSEC);
	printk("Parent thread releasing WR lock again\n");
	zassert_false(pthread_rwlock_unlock(&rwlock), "Failed to unlock");

	printk("\n3 threads acquire WR lock\n");
	printk("Main thread acquiring RD lock\n");

	ret = pthread_rwlock_timedrdlock(&rwlock, &time);

	if (ret != 0) {
		zassert_false(pthread_rwlock_rdlock(&rwlock), "Failed to lock");
	}

	printk("Main thread acquired RD lock\n");
	usleep(USEC_PER_MSEC);
	printk("Main thread releasing RD lock\n");
	zassert_false(pthread_rwlock_unlock(&rwlock), "Failed to unlock");

	for (i = 0; i < N_THR; i++) {
		zassert_false(pthread_join(newthread[i], &status),
			      "Failed to join");
	}

	zassert_false(pthread_rwlock_destroy(&rwlock),
		      "Failed to destroy rwlock");
}
