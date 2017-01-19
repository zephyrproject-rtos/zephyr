/* phil_fiber.c - dining philosopher */

/*
 * Copyright (c) 2011-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include "phil.h"

#define FORK(x) (&forks[x])
#define TAKE(x) nano_fiber_sem_take(x, TICKS_UNLIMITED)
#define GIVE(x) nano_fiber_sem_give(x)

#define RANDDELAY(x) my_delay(((sys_tick_get_32() * ((x) + 1)) & 0x1f) + 1)

/* externs */

extern struct nano_sem forks[N_PHILOSOPHERS];

/**
 *
 * @brief Wait for a number of ticks to elapse
 *
 * @param ticks   Number of ticks to delay.
 *
 * @return N/A
 */

static void my_delay(int ticks)
{
	struct nano_timer timer;

	nano_timer_init(&timer, (void *) 0);
	nano_fiber_timer_start(&timer, ticks);
	nano_fiber_timer_test(&timer, TICKS_UNLIMITED);
}

/**
 *
 * @brief Entry point to a philosopher's thread
 *
 * This routine runs as a task in the microkernel environment
 * and as a fiber in the nanokernel environment.
 *
 * @return N/A
 */

void phil_entry(void)
{
	int counter;
	struct nano_sem *f1;	/* fork #1 */
	struct nano_sem *f2;	/* fork #2 */
	static int myId;        /* next philosopher ID */
	int pri = irq_lock();   /* interrupt lock level */
	int id = myId++;        /* current philosopher ID */

	irq_unlock(pri);

	/* always take the lowest fork first */
	if ((id+1) != N_PHILOSOPHERS) {
		f1 = FORK(id);
		f2 = FORK(id + 1);
	} else {
		f1 = FORK(0);
		f2 = FORK(id);
	}

	for (counter = 0; counter < 5; counter++) {
		TAKE(f1);
		TAKE(f2);

		RANDDELAY(id);

		GIVE(f2);
		GIVE(f1);

		RANDDELAY(id);
	}
}
