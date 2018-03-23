/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <ztest.h>
#include <misc/printk.h>

#define STACK_SIZE 1024

sem_t sema;

static K_THREAD_STACK_DEFINE(stack, STACK_SIZE);

static void *foo_func(void *p1)
{
	printk("Child thread running\n");
	zassert_false(sem_post(&sema), "sem_post failed\n");
	return NULL;
}

static void test_sema(void)
{
	pthread_t newthread;
	pthread_attr_t attr;
	struct sched_param schedparam;
	int schedpolicy = SCHED_FIFO;
	int val;

	schedparam.priority = 1;

	pthread_attr_init(&attr);
	pthread_attr_setstack(&attr, &stack, STACK_SIZE);
	pthread_attr_setschedpolicy(&attr, schedpolicy);
	pthread_attr_setschedparam(&attr, &schedparam);

	zassert_equal(sem_init(&sema, 0, (CONFIG_SEM_VALUE_MAX + 1)), -1,
		      "value larger than %d\n", CONFIG_SEM_VALUE_MAX);
	zassert_equal(errno, EINVAL, NULL);

	zassert_false(sem_init(&sema, 0, 0), "sem_init failed\n");

	zassert_equal(sem_getvalue(&sema, &val), 0, NULL);
	zassert_equal(val, 0, NULL);

	pthread_create(&newthread, &attr, foo_func, NULL);
	zassert_false(sem_wait(&sema), "sem_wait failed\n");

	printk("Parent thread unlocked\n");
	zassert_false(sem_destroy(&sema), "sema is not destroyed\n");
}

/*
 * Test for named semaphores
 */

#define N_THRD (3)
#define SSZ (64)

const char sem_name[] = "/shared_sem";
int shared_counter = 0;

/*
 * Flags to track the status of child threads
 */
sem_t *ssem_value[N_THRD];
unsigned int status_flag_open[N_THRD];
unsigned int status_flag_count[N_THRD];
unsigned int status_flag_close[N_THRD];
unsigned int unlinked = 0;

/*
 * Simple test:
 * - all child threads receive the same semaphore
 * - can be used for shared variable access
 * - can be destroyed properly
 */
void *child_code(void *vid)
{
	unsigned id = (unsigned) vid;
	sem_t *ssem;
	int rv;

	/* Check open */
	ssem = sem_open(sem_name, O_CREAT, 0666, 1);
	ssem_value[id] = ssem;
	status_flag_open[id] = 1;

	/* Check correct locking and counter */
	rv = sem_wait(ssem);
	shared_counter++;
	rv = sem_post(ssem);
	status_flag_count[id] = 1;

	/* Check close */
	rv = sem_close(ssem);
	status_flag_close[id] = 1;

	/* Thread 0 takes care of unlinking */
	if (id == 0) {
		rv = sem_unlink(sem_name);
		unlinked = 1;
	}

	return NULL;
}

K_THREAD_DEFINE(t0, SSZ, child_code, (void *)0, NULL, NULL, K_USER,
		0, K_FOREVER);
K_THREAD_DEFINE(t1, SSZ, child_code, (void *)1, NULL, NULL, K_USER,
		0, K_FOREVER);
K_THREAD_DEFINE(t2, SSZ, child_code, (void *)2, NULL, NULL, K_USER,
		0, K_FOREVER);

/*
 * Checks if a status array is all set to 1.
 */
static inline bool check_barrier(unsigned int status[N_THRD])
{
	for (int i = 0; i < N_THRD; ++i) {
		if (status[i] == 0)
			return false;
	}

	return true;
}

/*
 * Checks if the semaphore contains the same value for all threads.
 */
static inline bool check_sem(sem_t *sarr[N_THRD])
{
	for (int i = 1; i < N_THRD; ++i) {
		if (sarr[0] != sarr[i])
			return false;
	}

	return true;
}


/*
 * This function behaves as a checker, and taking care all threads pass all
 * checks.
 * TODO: add timeout.
 */
static void test_named_semaphores(void)
{
	/* Launch threads */
	k_thread_start(t0);
	k_thread_start(t1);
	k_thread_start(t2);

	/* Wait for all to open and check they received the same object */
	while (!check_barrier(status_flag_open)) {
		k_sleep(100);
	}
	zassert_false(!check_sem(ssem_value),
		      "Wrong in shared semaphore open");

	/* Check that there is no race condition in variable rw op */
	while (!check_barrier(status_flag_count)) {
		k_sleep(100);
	}
	zassert_equal(shared_counter, N_THRD, "Shared variable access failed");

	while (!check_barrier(status_flag_close)) {
		k_sleep(100);
	}

	while (!unlinked) {
		k_sleep(100);
	}
}

void test_main(void)
{
	ztest_test_suite(test_sem, ztest_unit_test(test_sema),
			 ztest_unit_test(test_named_semaphores));
	ztest_run_test_suite(test_sem);
}
