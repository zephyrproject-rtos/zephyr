/* phil_thread.c - dining philosopher */

/*
 * Copyright (c) 2011-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include "phil.h"

#define FORK(x) (&forks[x])
#define TAKE(x) k_sem_take(x, K_FOREVER)
#define GIVE(x) k_sem_give(x)

#define RANDDELAY(x) k_sleep(10 * (x) + 1)

/* externs */

extern struct k_sem forks[N_PHILOSOPHERS];



/**
 *
 * @brief Entry point to a philosopher's thread
 *
 * @return N/A
 */

void phil_entry(void)
{
	int counter;
	struct k_sem *f1;       /* fork #1 */
	struct k_sem *f2;       /* fork #2 */
	static int myId;        /* next philosopher ID */
	int pri = irq_lock();   /* interrupt lock level */
	int id = myId++;        /* current philosopher ID */

	irq_unlock(pri);

	/* always take the lowest fork first */
	if ((id + 1) != N_PHILOSOPHERS) {
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
