/* main.c - Synchronisation demo */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>


/*
 * Nanokernel version of hello world demo has a task and a fiber that utilize
 * semaphores and timers to take turns printing a greeting message at
 * a controlled rate.
 */


/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

#define STACKSIZE 2000

char __stack fiberStack[STACKSIZE];

struct nano_sem nanoSemTask;
struct nano_sem nanoSemFiber;

void fiberEntry(void)
{
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};

	nano_sem_init(&nanoSemFiber);
	nano_timer_init(&timer, data);

	while (1) {
		/* wait for task to let us have a turn */
		nano_fiber_sem_take(&nanoSemFiber, TICKS_UNLIMITED);

		/* say "hello" */
		printk("%s: Hello World!\n", __func__);

		/* wait a while, then let task have a turn */
		nano_fiber_timer_start(&timer, SLEEPTICKS);
		nano_fiber_timer_test(&timer, TICKS_UNLIMITED);
		nano_fiber_sem_give(&nanoSemTask);
	}
}

void main(void)
{
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};

	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t) fiberEntry, 0, 0, 7, 0);

	nano_sem_init(&nanoSemTask);
	nano_timer_init(&timer, data);

	while (1) {
		/* say "hello" */
		printk("%s: Hello World!\n", __func__);

		/* wait a while, then let fiber have a turn */
		nano_task_timer_start(&timer, SLEEPTICKS);
		nano_task_timer_test(&timer, TICKS_UNLIMITED);
		nano_task_sem_give(&nanoSemFiber);

		/* now wait for fiber to let us have a turn */
		nano_task_sem_take(&nanoSemTask, TICKS_UNLIMITED);
	}
}

