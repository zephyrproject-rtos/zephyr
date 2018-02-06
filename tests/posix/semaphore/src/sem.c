/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <semaphore.h>
#include <ztest.h>

#define STACK_SIZE 1024

sem_t sema;

static K_THREAD_STACK_DEFINE(stack, STACK_SIZE);

static void *foo_func(void *p1)
{
	zassert_false(sem_wait(&sema), "sem_wait failed\n");
	printk("Print me after taking semaphore!\n");
	zassert_false(sem_post(&sema), "sem_post failed\n");
	return NULL;
}

static void test_sema(void)
{
	pthread_t newthread;
	pthread_attr_t attr;
	struct sched_param schedparam;
	int schedpolicy = SCHED_FIFO;

	schedparam.priority = 1;

	pthread_attr_init(&attr);
	pthread_attr_setstack(&attr, &stack, STACK_SIZE);
	pthread_attr_setschedpolicy(&attr, schedpolicy);
	pthread_attr_setschedparam(&attr, &schedparam);

	zassert_false(sem_init(&sema, 0, 1), "sem_init failed\n");

	pthread_create(&newthread, &attr, foo_func, NULL);

	printk("after releasing sem\n");
	zassert_false(sem_wait(&sema), "sem_wait failed\n");
	printk("After taking semaphore second time\n");

	zassert_false(sem_post(&sema), "sem_post failed\n");
}

void test_main(void)
{
	ztest_test_suite(test_sem, ztest_unit_test(test_sema));
	ztest_run_test_suite(test_sem);
}
