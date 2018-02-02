/*
 * Copyright (c) 2018 Juan Manuel Torres Palma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include <kernel.h>
#include <semaphore.h>
#include <misc/printk.h>

#define STACKSZ 256
#define NTH 2
#define CTR_LIM 70

K_SEM_DEFINE(old_sem, 0, NTH);

static sem_t semaphore;
static bool finished; /* Assume false */
static int shared_counter;
static int tot_counter[NTH];
static int th_status[NTH];

void *thread_code(void *vid)
{
	int id = (int)vid;
	int wait_ms = (id + 1) * 40;
	bool sleep_fl = false;

	/* Use a zephyr semaphore to wait for main initialization */
	k_sem_take(&old_sem, K_FOREVER);

	printk("Thread %d running\n", id);

	do {
		sem_wait(&semaphore);
		if (shared_counter != CTR_LIM) {
			if (++shared_counter == CTR_LIM)
				finished = true;
			++tot_counter[id];
			sleep_fl = true;
		}
		sem_post(&semaphore);

		/* If could write, go to sleep */
		if (sleep_fl && !finished) {
			k_sleep(wait_ms);
			sleep_fl = false;
		}

	} while (!finished);

	printk("Thread %d finished\n", id);
	th_status[id] = 1;

	return NULL;
}

int wait_for_threads(void)
{
	int i;

	for (i = 0; i < NTH; i++) {
		if (!th_status[i]) {
			return 1;
		}
	}

	return 0;
}

int check_result(void)
{
	int i;
	int sum = 0;

	for (i = 0; i < NTH; ++i) {
		sum += tot_counter[i];
		printk("Thread %d counter: %d\n", i, tot_counter[i]);
	}

	return (sum == shared_counter) && (sum == CTR_LIM);
}

void main(void)
{
	int status = TC_FAIL;
	int i;

	TC_START("POSIX semaphore APIs\n");

	sem_init(&semaphore, 0, 1);

	for (i = 0; i < NTH; ++i)
		k_sem_give(&old_sem);

	while (wait_for_threads()) {
		k_sleep(100);
	}

	sem_destroy(&semaphore);

	if (!check_result()) {
		TC_END_REPORT(status);
	}

	printk("Test finished\n");

	status = TC_PASS;
}

K_THREAD_DEFINE(thread0, STACKSZ, thread_code, (void *)0, NULL, NULL,
		K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);

K_THREAD_DEFINE(thread1, STACKSZ, thread_code, (void *)1, NULL, NULL,
		K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);
