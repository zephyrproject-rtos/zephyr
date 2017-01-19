/* sema_r.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "receiver.h"
#include "master.h"

#ifdef SEMA_BENCH

/* semaphore signal speed test */

/**
 *
 * @brief Receive task (Wait task)
 *
 * @return N/A
 */
void waittask(void)
{
	int i;

	ksem_t slist[5];

	slist[0] = SEM1;
	slist[1] = SEM2;
	slist[2] = ENDLIST;
	slist[3] = ENDLIST;
	slist[4] = ENDLIST;

	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_take(SEM1, TICKS_UNLIMITED);
	}
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_take(SEM1, SEMA_WAIT_TIME);
	}

	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take(slist, TICKS_UNLIMITED);
	}
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take(slist, SEMA_WAIT_TIME);
	}

	slist[2] = SEM3;
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take(slist, TICKS_UNLIMITED);
	}
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take(slist, SEMA_WAIT_TIME);
	}

	slist[3] = SEM4;
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take(slist, TICKS_UNLIMITED);
	}
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take(slist, SEMA_WAIT_TIME);
	}
}

#endif /* SEMA_BENCH */
