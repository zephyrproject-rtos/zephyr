/* phil_task.c - dining philosophers */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
#include <arch/cpu.h>
#include "phil.h"
#else
#include <vxmicro.h>
#include "phil.h"
#endif

/* defines */

#define DEMO_DESCRIPTION  \
	"\x1b[2J\x1b[15;1H"   \
	"Demo Description\n"  \
	"----------------\n"  \
	"An implementation of a solution to the Dining Philosophers problem\n"  \
	"(a classic multi-thread synchronization problem).  This particular\n"  \
	"implementation demonstrates the usage of multiple (6) %s\n"            \
	"of differing priorities and the %s semaphores and timers."

#ifdef CONFIG_NANOKERNEL

#define STSIZE 1024

/* externs */

extern void philEntry(void);

/* globals */

char __stack philStack[N_PHILOSOPHERS][STSIZE];
struct nano_sem forks[N_PHILOSOPHERS];
#endif  /*  CONFIG_NANOKERNEL */

#ifdef CONFIG_NANOKERNEL
/*******************************************************************************
*
* main - nanokernel entry point
*
* RETURNS: does not return
*/

int main(void)
{
	int i;

	PRINTF(DEMO_DESCRIPTION, "fibers", "nanokernel");

	for (i = 0; i < N_PHILOSOPHERS; i++) {
		nano_sem_init(&forks[i]);
		nano_task_sem_give(&forks[i]);
	}

	/* create philosopher fibers */
	for (i = 0; i < N_PHILOSOPHERS; i++) {
		task_fiber_start(&philStack[i][0], STSIZE,
						(nano_fiber_entry_t) philEntry, 0, 0, 6, 0);
	}

	/* wait forever */
	while (1) {
		extern void nano_cpu_idle(void);
		nano_cpu_idle();
	}
}

#else
/*******************************************************************************
*
* philDemo - routine to start dining philosopher demo
*
* RETURNS: does not return
*/

void philDemo(void)
{
	PRINTF(DEMO_DESCRIPTION, "tasks", "microkernel");

	task_group_start(PHI);

	/* wait forever */
	while (1) {
		task_sleep(10000);
	}
}
#endif
