/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <pthread.h>
#include <semaphore.h>

#define N_THR 3

#define BOUNCES 64

#define STACKSZ 1024

K_THREAD_STACK_ARRAY_DEFINE(stacks, N_THR, STACKSZ);

void *thread_top(void *p1);

PTHREAD_MUTEX_DEFINE(lock);

PTHREAD_COND_DEFINE(cvar0);

PTHREAD_COND_DEFINE(cvar1);

PTHREAD_BARRIER_DEFINE(barrier, N_THR);

sem_t main_sem;

static int bounce_failed;
static int bounce_done[N_THR];

static int curr_bounce_thread;

static int barrier_failed;
static int barrier_done[N_THR];

/* First phase bounces execution between two threads using a condition
 * variable, continuously testing that no other thread is mucking with
 * the protected state.  This ends with all threads going back to
 * sleep on the condition variable and being woken by main() for the
 * second phase.
 *
 * Second phase simply lines up all the threads on a barrier, verifies
 * that none run until the last one enters, and that all run after the
 * exit.
 *
 * Test success is signaled to main() using a traditional semaphore.
 */

void *thread_top(void *p1)
{
	int i, j, id = (int) p1;
	int policy;
	struct sched_param schedparam;

	pthread_getschedparam(pthread_self(), &policy, &schedparam);
	printk("Thread %d starting with scheduling policy %d & priority %d\n",
		 id, policy, schedparam.priority);
	/* Try a double-lock here to exercise the failing case of
	 * trylock.  We don't support RECURSIVE locks, so this is
	 * guaranteed to fail.
	 */
	pthread_mutex_lock(&lock);

	if (!pthread_mutex_trylock(&lock)) {
		printk("pthread_mutex_trylock inexplicably succeeded\n");
		bounce_failed = 1;
	}

	pthread_mutex_unlock(&lock);

	for (i = 0; i < BOUNCES; i++) {

		pthread_mutex_lock(&lock);

		/* Wait for the current owner to signal us, unless we
		 * are the very first thread, in which case we need to
		 * wait a bit to be sure the other threads get
		 * scheduled and wait on cvar0.
		 */
		if (!(id == 0 && i == 0)) {
			pthread_cond_wait(&cvar0, &lock);
		} else {
			pthread_mutex_unlock(&lock);
			usleep(500 * USEC_PER_MSEC);
			pthread_mutex_lock(&lock);
		}

		/* Claim ownership, then try really hard to give someone
		 * else a shot at hitting this if they are racing.
		 */
		curr_bounce_thread = id;
		for (j = 0; j < 1000; j++) {
			if (curr_bounce_thread != id) {
				printk("Racing bounce threads\n");
				bounce_failed = 1;
				sem_post(&main_sem);
				pthread_mutex_unlock(&lock);
				return NULL;
			}
			sched_yield();
		}

		/* Next one's turn, go back to the top and wait.  */
		pthread_cond_signal(&cvar0);
		pthread_mutex_unlock(&lock);
	}

	/* Signal we are complete to main(), then let it wake us up.  Note
	 * that we are using the same mutex with both cvar0 and cvar1,
	 * which is non-standard but kosher per POSIX (and it works fine
	 * in our implementation
	 */
	pthread_mutex_lock(&lock);
	bounce_done[id] = 1;
	sem_post(&main_sem);
	pthread_cond_wait(&cvar1, &lock);
	pthread_mutex_unlock(&lock);

	/* Now just wait on the barrier.  Make sure no one else finished
	 * before we wait on it, then signal that we're done
	 */
	for (i = 0; i < N_THR; i++) {
		if (barrier_done[i]) {
			printk("Barrier exited early\n");
			barrier_failed = 1;
			sem_post(&main_sem);
		}
	}
	pthread_barrier_wait(&barrier);
	barrier_done[id] = 1;
	sem_post(&main_sem);
	pthread_exit(p1);

	return NULL;
}

int bounce_test_done(void)
{
	int i;

	if (bounce_failed) {
		return 1;
	}

	for (i = 0; i < N_THR; i++) {
		if (!bounce_done[i]) {
			return 0;
		}
	}

	return 1;
}

int barrier_test_done(void)
{
	int i;

	if (barrier_failed) {
		return 1;
	}

	for (i = 0; i < N_THR; i++) {
		if (!barrier_done[i]) {
			return 0;
		}
	}

	return 1;
}

void test_pthread(void)
{
	int i, ret, min_prio, max_prio;
	pthread_attr_t attr[N_THR];
	struct sched_param schedparam;
	pthread_t newthread[N_THR];
	int schedpolicy = SCHED_FIFO;
	void *retval;

	sem_init(&main_sem, 0, 1);
	printk("POSIX thread IPC APIs\n");
	schedparam.priority = CONFIG_NUM_COOP_PRIORITIES - 1;
	min_prio = sched_get_priority_min(schedpolicy);
	max_prio = sched_get_priority_max(schedpolicy);

	ret = (min_prio < 0 || max_prio < 0 ||
			schedparam.priority < min_prio ||
			schedparam.priority > max_prio);

	/*TESTPOINT: Check if scheduling priority is valid*/
	zassert_false(ret,
			"Scheduling priority outside valid priority range");

	for (i = 0; i < N_THR; i++) {
		ret = pthread_attr_init(&attr[i]);
		if (ret != 0) {
			zassert_false(pthread_attr_destroy(&attr[i]),
				      "Unable to destroy pthread object attrib");
			zassert_false(pthread_attr_init(&attr[i]),
				      "Unable to create pthread object attrib");
		}

		pthread_attr_setstack(&attr[i], &stacks[i][0], STACKSZ);
		pthread_attr_setschedpolicy(&attr[i], schedpolicy);
		pthread_attr_setschedparam(&attr[i], &schedparam);
		ret = pthread_create(&newthread[i], &attr[i], thread_top,
				     (void *)i);

		/*TESTPOINT: Check if thread is created successfully*/
		zassert_false(ret, "Number of threads exceed max limit");
	}

	while (!bounce_test_done()) {
		sem_wait(&main_sem);
	}

	/*TESTPOINT: Check if bounce test passes*/
	zassert_false(bounce_failed, "Bounce test failed");

	printk("Bounce test OK\n");

	/* Wake up the worker threads */
	pthread_mutex_lock(&lock);
	pthread_cond_broadcast(&cvar1);
	pthread_mutex_unlock(&lock);

	while (!barrier_test_done()) {
		sem_wait(&main_sem);
	}

	/*TESTPOINT: Check if barrier test passes*/
	zassert_false(barrier_failed, "Barrier test failed");

	for (i = 0; i < N_THR; i++) {
		pthread_join(newthread[i], &retval);
	}

	printk("Barrier test OK\n");
}

void test_main(void)
{
	ztest_test_suite(test_pthreads,
			ztest_unit_test(test_pthread));
	ztest_run_test_suite(test_pthreads);
}
