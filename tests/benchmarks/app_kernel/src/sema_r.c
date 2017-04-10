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

	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		k_sem_take(&SEM1, K_FOREVER);
	}
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		k_sem_take(&SEM1, SEMA_WAIT_TIME);
	}

}

#endif /* SEMA_BENCH */
