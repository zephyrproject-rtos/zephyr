/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include <kernel.h>
#include <pthread.h>

#define N_THR 3

#define BOUNCES 64

#define STACKSZ 1024

K_THREAD_STACK_ARRAY_DEFINE(stacks, N_THR, STACKSZ);

void *thread_top(void *p1);

PTHREAD_MUTEX_DEFINE(lock);

PTHREAD_COND_DEFINE(cvar0);

PTHREAD_COND_DEFINE(cvar1);

PTHREAD_BARRIER_DEFINE(barrier, N_THR);

K_SEM_DEFINE(main_sem, 0, 2*N_THR);

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
	TC_PRINT("Thread %d starting with scheduling policy %d & priority %d\n",
		 id, policy, schedparam.priority);
	/* Try a double-lock here to exercise the failing case of
	 * trylock.  We don't support RECURSIVE locks, so this is
	 * guaranteed to fail.
	 */
	pthread_mutex_lock(&lock);

	if (!pthread_mutex_trylock(&lock)) {
		TC_ERROR("pthread_mutex_trylock inexplicably succeeded\n");
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
				TC_ERROR("Racing bounce threads\n");
				bounce_failed = 1;
				k_sem_give(&main_sem);
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
	k_sem_give(&main_sem);
	pthread_cond_wait(&cvar1, &lock);
	pthread_mutex_unlock(&lock);

	/* Now just wait on the barrier.  Make sure no one else finished
	 * before we wait on it, then signal that we're done
	 */
	for (i = 0; i < N_THR; i++) {
		if (barrier_done[i]) {
			TC_ERROR("Barrier exited early\n");
			barrier_failed = 1;
			k_sem_give(&main_sem);
		}
	}
	pthread_barrier_wait(&barrier);
	barrier_done[id] = 1;
	k_sem_give(&main_sem);
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

void main(void)
{
	int i, ret, min_prio, max_prio, status = TC_FAIL;
	pthread_attr_t attr[N_THR];
	struct sched_param schedparam;
	pthread_t newthread[N_THR];
	int schedpolicy = SCHED_FIFO;
	void *retval;

	TC_START("POSIX thread IPC APIs\n");
	schedparam.priority = CONFIG_NUM_COOP_PRIORITIES - 1;
	min_prio = sched_get_priority_min(schedpolicy);
	max_prio = sched_get_priority_max(schedpolicy);

	if (min_prio < 0 || max_prio < 0 || schedparam.priority < min_prio ||
	    schedparam.priority > max_prio) {
		TC_ERROR("Scheduling priority outside valid priority range\n");
		goto done;
	}

	for (i = 0; i < N_THR; i++) {
		ret = pthread_attr_init(&attr[i]);
		if (ret != 0) {
			TC_ERROR("Thread attribute initialization failed\n");
			goto done;
		}
		pthread_attr_setstack(&attr[i], &stacks[i][0], STACKSZ);
		pthread_attr_setschedpolicy(&attr[i], schedpolicy);
		pthread_attr_setschedparam(&attr[i], &schedparam);
		ret = pthread_create(&newthread[i], &attr[i], thread_top,
				     (void *)i);

		if (ret != 0) {
			TC_ERROR("Number of threads exceeds maximum limit\n");
			goto done;
		}

	}

	while (!bounce_test_done()) {
		k_sem_take(&main_sem, K_FOREVER);
	}

	if (bounce_failed) {
		goto done;
	}

	TC_PRINT("Bounce test OK\n");

	/* Wake up the worker threads */
	pthread_mutex_lock(&lock);
	pthread_cond_broadcast(&cvar1);
	pthread_mutex_unlock(&lock);

	while (!barrier_test_done()) {
		k_sem_take(&main_sem, K_FOREVER);
	}

	if (barrier_failed) {
		goto done;
	}

	for (i = 0; i < N_THR; i++) {
		pthread_join(newthread[i], &retval);
	}

	TC_PRINT("Barrier test OK\n");
	status = TC_PASS;

 done:
	TC_END_REPORT(status);
}
