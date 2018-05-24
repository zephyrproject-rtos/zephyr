/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <misc/printk.h>

#define STACK_SIZE 1024

sem_t sema;

static K_THREAD_STACK_DEFINE(stack, STACK_SIZE);

static void *foo_func(void *p1)
{
	printk("Child thread running\n");
	zassert_false(sem_post(&sema), "sem_post failed");
	return NULL;
}

static void test_sema(void)
{
	pthread_t newthread;
	pthread_attr_t attr;
	struct sched_param schedparam;
	int schedpolicy = SCHED_FIFO;
	int val, ret;

	schedparam.priority = 1;
	ret = pthread_attr_init(&attr);
	if (ret != 0) {
		zassert_false(pthread_attr_destroy(&attr),
			      "Unable to destroy pthread object attrib");
		zassert_false(pthread_attr_init(&attr),
			      "Unable to create pthread object attrib");
	}

	pthread_attr_setstack(&attr, &stack, STACK_SIZE);
	pthread_attr_setschedpolicy(&attr, schedpolicy);
	pthread_attr_setschedparam(&attr, &schedparam);

	zassert_equal(sem_init(&sema, 0, (CONFIG_SEM_VALUE_MAX + 1)), -1,
		      "value larger than %d\n", CONFIG_SEM_VALUE_MAX);
	zassert_equal(errno, EINVAL, NULL);

	zassert_false(sem_init(&sema, 0, 0), "sem_init failed");

	zassert_equal(sem_getvalue(&sema, &val), 0, NULL);
	zassert_equal(val, 0, NULL);

	ret = pthread_create(&newthread, &attr, foo_func, NULL);
	zassert_false(ret, "Thread creation failed");

	zassert_false(sem_wait(&sema), "sem_wait failed");

	printk("Parent thread unlocked\n");
	zassert_false(sem_destroy(&sema), "sema is not destroyed");
}

void test_main(void)
{
	ztest_test_suite(test_sem, ztest_unit_test(test_sema));
	ztest_run_test_suite(test_sem);
}
