/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define NUM_THREADS 2

#define STACK_SIZE 4096

#define TIME_SLICE_SIZE 10   /* Measured in msec */

static struct k_sem sems[NUM_THREADS];
static struct k_thread threads[NUM_THREADS];
static uint32_t counters[NUM_THREADS];

K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_THREADS, 4096);

void thread_entry(void *arg1, void *arg2, void *arg3)
{
	struct k_sem *take_sem = arg1;
	struct k_sem *give_sem = arg2;
	uint32_t *counter = arg3;

	while (1) {
		k_sem_take(take_sem, K_FOREVER);

		(*counter)++;

		k_sem_give(give_sem);
	}
}

int main(void)
{
	int  priority;
	uint32_t i;
	uint32_t sum;
	uint32_t elapsed = 0;

	priority = k_thread_priority_get(k_current_get());

	for (i = 0; i < NUM_THREADS; i++) {
		k_sem_init(&sems[i], 0, 1);
	}

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&threads[i], stacks[i], STACK_SIZE,
				thread_entry, &sems[i],
				&sems[(i + 1) % NUM_THREADS], &counters[i],
				priority + 1, 0, K_NO_WAIT);
	}

#ifdef CONFIG_TIMESLICING
	k_sched_time_slice_set(TIME_SLICE_SIZE, 0);
#endif

	k_sem_give(&sems[0]);

	while (1) {
		k_sleep(K_SECONDS(30));

		elapsed += 30;
		sum = 0;
		for (i = 0; i < NUM_THREADS; i++) {
			sum += counters[i];
		}

		printf(" *** Time Interval ending @ %u seconds ***\n", elapsed);
		printf("    Total number of ctx switches: %u\n", sum);
	}

	return 0;
}
