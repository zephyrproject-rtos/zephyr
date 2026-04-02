/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#define NUM_THREADS 3
#define TCOUNT 10
#define COUNT_LIMIT 12

static int count;

K_MUTEX_DEFINE(count_mutex);
K_CONDVAR_DEFINE(count_threshold_cv);

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

K_THREAD_STACK_ARRAY_DEFINE(tstacks, NUM_THREADS, STACK_SIZE);

static struct k_thread t[NUM_THREADS];

void inc_count(void *p1, void *p2, void *p3)
{
	int i;
	long my_id = (long)p1;

	for (i = 0; i < TCOUNT; i++) {
		k_mutex_lock(&count_mutex, K_FOREVER);
		count++;

		/*
		 * Check the value of count and signal waiting thread when
		 * condition is reached.  Note that this occurs while mutex is
		 * locked.
		 */

		if (count == COUNT_LIMIT) {
			printk("%s: thread %ld, count = %d  Threshold reached.",
			       __func__, my_id, count);
			k_condvar_signal(&count_threshold_cv);
			printk("Just sent signal.\n");
		}
		printk("%s: thread %ld, count = %d, unlocking mutex\n",
		       __func__, my_id, count);
		k_mutex_unlock(&count_mutex);

		/* Sleep so threads can alternate on mutex lock */
		k_sleep(K_MSEC(500));
	}
}

void watch_count(void *p1, void *p2, void *p3)
{
	long my_id = (long)p1;

	printk("Starting %s: thread %ld\n", __func__, my_id);

	k_mutex_lock(&count_mutex, K_FOREVER);
	while (count < COUNT_LIMIT) {
		printk("%s: thread %ld Count= %d. Going into wait...\n",
		       __func__, my_id, count);
		k_condvar_wait(&count_threshold_cv, &count_mutex, K_FOREVER);

		printk("%s: thread %ld Condition signal received. Count= %d\n",
		       __func__, my_id, count);
	}
	printk("%s: thread %ld Updating the value of count...\n",
	       __func__, my_id);
	count += 125;
	printk("%s: thread %ld count now = %d.\n", __func__, my_id, count);
	printk("%s: thread %ld Unlocking mutex.\n", __func__, my_id);
	k_mutex_unlock(&count_mutex);
}

int main(void)
{
	long t1 = 1, t2 = 2, t3 = 3;
	int i;

	count = 0;

	k_thread_create(&t[0], tstacks[0], STACK_SIZE, watch_count,
			INT_TO_POINTER(t1), NULL, NULL, K_PRIO_PREEMPT(10), 0,
			K_NO_WAIT);

	k_thread_create(&t[1], tstacks[1], STACK_SIZE, inc_count,
			INT_TO_POINTER(t2), NULL, NULL, K_PRIO_PREEMPT(10), 0,
			K_NO_WAIT);

	k_thread_create(&t[2], tstacks[2], STACK_SIZE, inc_count,
			INT_TO_POINTER(t3), NULL, NULL, K_PRIO_PREEMPT(10), 0,
			K_NO_WAIT);

	/* Wait for all threads to complete */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_join(&t[i], K_FOREVER);
	}

	printk("Main(): Waited and joined with %d threads. Final value of count = %d. Done.\n",
	       NUM_THREADS, count);
	return 0;
}
