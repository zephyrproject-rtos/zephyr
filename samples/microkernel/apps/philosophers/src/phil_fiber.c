/* phil_fiber.c - dining philosopher */

/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
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
/* includes */

#ifdef CONFIG_NANOKERNEL
  #include <nanokernel.h>
  #include "phil.h"
#else  /* ! CONFIG_NANOKERNEL */
  #include <vxmicro.h>
  #include "phil.h"
#endif /*  CONFIG_NANOKERNEL */

#include <nanokernel/cpu.h>	/* irq_lock/irq_unlock */

/* defines */

#ifdef CONFIG_NANOKERNEL
  #define FORK(x) &forks[x]
  #define TAKE(x) nano_fiber_sem_take_wait(x)
  #define GIVE(x) nano_fiber_sem_give(x)
  #define RANDDELAY(x) myDelay(((nano_tick_get_32() * ((x) +1)) & 0x1f) + 1)
#else  /* ! CONFIG_NANOKERNEL */
  #define FORK(x) forks[x]
  #define TAKE(x) task_mutex_lock_wait(x)
  #define GIVE(x) task_mutex_unlock(x)
  #define RANDDELAY(x) myDelay(((task_tick_get_32() * ((x) +1)) & 0x1f) + 1)
#endif /*  CONFIG_NANOKERNEL */

#define PRINT(x,y)	myPrint(x,y)

#ifdef CONFIG_NANOKERNEL
/* externs */

extern struct nano_sem forks[N_PHILOSOPHERS];
#else  /* ! CONFIG_NANOKERNEL */
/* globals */

kmutex_t forks[] = {forkMutex0, forkMutex1, forkMutex2, forkMutex3, forkMutex4, forkMutex5};
#endif /*  CONFIG_NANOKERNEL */

/*******************************************************************************
*
* myPrint - print a philosophers state
*
* RETURNS: N/A
*/

static void myPrint
	(
	int   id,    /* philosopher ID */
	char *str    /* EATING or THINKING */
	)
	{
	PRINTF ("\x1b[%d;%dHPhilosopher %d %s\n", id + 1, 1, id, str);
	}

/*******************************************************************************
*
* myDelay - wait for a number of ticks to elapse
*
* RETURNS: N/A
*/

static void myDelay
	(
	int ticks    /* # of ticks to delay */
	)
	{
#ifdef CONFIG_MICROKERNEL
	task_sleep (ticks);
#else
	struct nano_timer timer;

	nano_timer_init  (&timer, (void *) 0);
	nano_fiber_timer_start (&timer, ticks);
	nano_fiber_timer_wait  (&timer);
#endif
	}

/*******************************************************************************
*
* philEntry - entry point to a philosopher's thread
*
* This routine runs as a task in the microkernel environment
* and as a fiber in the nanokernel environment.
*
* RETURNS: N/A
*/

void philEntry (void)
	{
#ifdef CONFIG_NANOKERNEL
	struct nano_sem *f1;	/* fork #1 */
	struct nano_sem *f2;	/* fork #2 */
#else
	kmutex_t f1;		/* fork #1 */
	kmutex_t f2;		/* fork #2 */
#endif
	static int myId;                /* next philosopher ID */
	int pri = irq_lock ();    /* interrupt lock level */
	int id = myId++;                /* current philosopher ID */

	irq_unlock (pri);

    /* always take the lowest fork first */
	if ((id+1) != N_PHILOSOPHERS) {
	f1 = FORK (id);
	f2 = FORK (id + 1);
	}
	else {
	f1 = FORK (0);
	f2 = FORK (id);
	}

	while (1) {
	TAKE (f1);
	TAKE (f2);

		PRINT(id, "EATING  ");
	RANDDELAY (id);

	GIVE (f2);
	GIVE (f1);

		PRINT(id, "THINKING");
	RANDDELAY (id);
	}
	}
