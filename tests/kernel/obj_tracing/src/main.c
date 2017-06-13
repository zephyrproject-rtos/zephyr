/* phil_task.c - dining philosophers */

/*
 * Copyright (c) 2011-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include "phil.h"

#define STSIZE 1024

extern void phil_entry(void);
extern void object_monitor(void);

K_THREAD_STACK_ARRAY_DEFINE(phil_stack, N_PHILOSOPHERS, STSIZE);
static struct k_thread phil_data[N_PHILOSOPHERS];
K_THREAD_STACK_DEFINE(mon_stack, STSIZE);
static struct k_thread mon_data;
struct k_sem forks[N_PHILOSOPHERS];

int main(void)
{
	int i;

	for (i = 0; i < N_PHILOSOPHERS; i++) {
		k_sem_init(&forks[i], 0, 1);
		k_sem_give(&forks[i]);
	}

	/* create philosopher threads */
	for (i = 0; i < N_PHILOSOPHERS; i++) {
		k_thread_create(&phil_data[i], &phil_stack[i][0], STSIZE,
				(k_thread_entry_t)phil_entry, NULL, NULL, NULL,
				K_PRIO_COOP(6), 0, K_NO_WAIT);
	}

	/* create object counter monitor thread */
	k_thread_create(&mon_data, mon_stack, STSIZE,
			(k_thread_entry_t)object_monitor, NULL, NULL, NULL,
			K_PRIO_COOP(7), 0, K_NO_WAIT);

	return 0;
}
