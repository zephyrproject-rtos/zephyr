/* phil_fiber.c - dining philosopher */

/*
 * Copyright (c) 2011-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include "phil.h"

#define FORK(x) forks[x]
#define TAKE(x) task_mutex_lock(x, TICKS_UNLIMITED)
#define GIVE(x) task_mutex_unlock(x)

#define RANDDELAY(x) task_sleep(((sys_tick_get_32() * ((x) + 1)) & 0x1f) + 1)

kmutex_t forks[] = {fork_mutex0, fork_mutex1, fork_mutex2, fork_mutex3, fork_mutex4};

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
	kmutex_t f1;		/* fork #1 */
	kmutex_t f2;		/* fork #2 */
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
