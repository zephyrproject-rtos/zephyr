/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>

#define THREAD_COUNT 2
#define THREAD_STACK_SIZE 1024

static struct k_thread thread_array[THREAD_COUNT];
K_THREAD_STACK_ARRAY_DEFINE(thread_stack, THREAD_COUNT, THREAD_STACK_SIZE);

static struct k_thread sem_thread;
K_THREAD_STACK_DEFINE(sem_thread_stack, THREAD_STACK_SIZE);

static struct k_mutex mutex;
static struct k_sem sem;
static struct k_timer timer;

/* Stop all testing threads */
static void clean_up(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	int i;

	for (i = 0; i < THREAD_COUNT; i++) {
		k_thread_abort(&thread_array[i]);
	}
	k_thread_abort(&sem_thread);
}

static void thread_func(void *p1, void *p2, void *p3)
{
	int ret, index;

	index = (intptr_t)p1;
	/* Wait for all testing threads started,
	 * and compete to get mutex simultaneously.
	 */
	k_sleep(K_MSEC(100));

	while (1) {
		ret = k_mutex_lock(&mutex, K_FOREVER);
		if (ret != 0) {
			printk("thread %d invokes k_mutex_lock failed with "
				"ret = %d\n", index, ret);
			ztest_test_fail();
			clean_up(NULL);
		}

		ret = k_sem_take(&sem, K_FOREVER);
		if (ret != 0) {
			printk("thread %d invokes k_sem_take failed with "
				"ret = %d\n", index, ret);
			ztest_test_fail();
			clean_up(NULL);
		}

		k_mutex_unlock(&mutex);
	}
}

static void sem_give_func(void *p1, void *p2, void *p3)
{
	while (1) {
		if (sem.count == 0) {
			k_sem_give(&sem);
		}
		/* Interval for semaphore release operation,
		 * make the test run fastly.
		 */
		k_sleep(K_MSEC(2));
	}
}

/**
 * @brief Stress test for take and release operations of mutex and semaphore.
 * @details Two threads will compete to get a common mutex and semaphore, and
 * run about 1 hour except for meeting an error.
 */
void test_mutex_stress(void)
{
	int i;

	k_mutex_init(&mutex);
	k_sem_init(&sem, 0, 1);

	/* testcase running time */
	k_timer_init(&timer, clean_up, NULL);
	k_timer_start(&timer, K_HOURS(1), K_NO_WAIT);

	for (i = 0; i < THREAD_COUNT; i++) {
		k_thread_create(&thread_array[i], thread_stack[i],
				K_THREAD_STACK_SIZEOF(thread_stack[i]),
				thread_func, (void *)(intptr_t)i, NULL, NULL,
				K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}
	k_thread_create(&sem_thread, sem_thread_stack,
			K_THREAD_STACK_SIZEOF(sem_thread_stack),
			sem_give_func, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	for (i = 0; i < THREAD_COUNT; i++) {
		k_thread_join(&thread_array[i], K_FOREVER);
	}
}

/* test case main entry */
void test_main(void)
{
	ztest_test_suite(mutex_stress_test,
			ztest_unit_test(test_mutex_stress));
	ztest_run_test_suite(mutex_stress_test);
}
