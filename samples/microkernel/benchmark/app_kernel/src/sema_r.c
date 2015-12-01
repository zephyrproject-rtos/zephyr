/* sema_r.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
