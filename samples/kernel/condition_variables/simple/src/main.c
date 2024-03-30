/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/arch_interface.h>

#define NUM_THREADS 20
#define STACK_SIZE (1024)

K_THREAD_STACK_ARRAY_DEFINE(tstacks, NUM_THREADS, STACK_SIZE);

static struct k_thread t[NUM_THREADS];

K_MUTEX_DEFINE(mutex);
K_CONDVAR_DEFINE(condvar);

static int done;

void worker_thread(void *p1, void *p2, void *p3)
{
	const int myid = (long)p1;
	const int workloops = 5;

	for (int i = 0; i < workloops; i++) {
		printk("[thread %d] working (%d/%d)\n", myid, i, workloops);
		k_sleep(K_MSEC(500));
	}

	/*
	 * we're going to manipulate done and use the cond, so we need the mutex
	 */
	k_mutex_lock(&mutex, K_FOREVER);

	/*
	 * increase the count of threads that have finished their work.
	 */
	done++;
	printk("[thread %d] done is now %d. Signalling cond.\n", myid, done);

	k_condvar_signal(&condvar);
	k_mutex_unlock(&mutex);
}

int main(void)
{
	k_tid_t tid[NUM_THREADS];

	done = 0;

	for (int i = 0; i < NUM_THREADS; i++) {
		tid[i] =
			k_thread_create(&t[i], tstacks[i], STACK_SIZE,
					worker_thread, INT_TO_POINTER(i), NULL,
					NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
	}
	k_sleep(K_MSEC(1000));

	k_mutex_lock(&mutex, K_FOREVER);

	/*
	 * are the other threads still busy?
	 */
	while (done < NUM_THREADS) {
		printk("[thread %s] done is %d which is < %d so waiting on cond\n",
		       __func__, done, (int)NUM_THREADS);

		/* block this thread until another thread signals cond. While
		 * blocked, the mutex is released, then re-acquired before this
		 * thread is woken up and the call returns.
		 */
		k_condvar_wait(&condvar, &mutex, K_FOREVER);

		printk("[thread %s] wake - cond was signalled.\n", __func__);

		/* we go around the loop with the lock held */
	}

	printk("[thread %s] done == %d so everyone is done\n",
		__func__, (int)NUM_THREADS);

	k_mutex_unlock(&mutex);
	return 0;
}
