/* sema_r.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "receiver.h"
#include "master.h"

#ifdef SEMA_BENCH

/* semaphore signal speed test */

/**
 *
 * waittask - receive task (Wait task)
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
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
		task_sem_take_wait(SEM1);
	}
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_take_wait_timeout(SEM1, SEMA_WAIT_TIME);
	}

	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take_wait(slist);
	}
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take_wait_timeout(slist, SEMA_WAIT_TIME);
	}

	slist[2] = SEM3;
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take_wait(slist);
	}
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take_wait_timeout(slist, SEMA_WAIT_TIME);
	}

	slist[3] = SEM4;
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take_wait(slist);
	}
	for (i = 0; i < NR_OF_SEMA_RUNS; i++) {
		task_sem_group_take_wait_timeout(slist, SEMA_WAIT_TIME);
	}
}

#endif /* SEMA_BENCH */
