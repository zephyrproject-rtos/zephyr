/*! @file
 *  @brief Solution to the dining philosophers problem using fibers.
 */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

#ifdef CONFIG_NANOKERNEL
/* For the nanokernel. */
#include <nanokernel.h>
#include "phil.h"
#else
/* For the microkernel. */
#include <zephyr.h>
#include "phil.h"
#endif

#include <nanokernel/cpu.h>	//!< Used to be know as: irq_lock/irq_unlock

#ifdef CONFIG_NANOKERNEL
/* For the nanokernel. */
#define FORK(x) &forks[x]
#define TAKE(x) nano_fiber_sem_take_wait(x)
#define GIVE(x) nano_fiber_sem_give(x)
#define RANDDELAY(x) myDelay(((nano_node_tick_get_32() * ((x) +1)) & 0x1f) + 1)
#else
/* For the microkernel. */
#define FORK(x) forks[x]
#define TAKE(x) task_mutex_lock_wait(x)
#define GIVE(x) task_mutex_unlock(x)
#define RANDDELAY(x) myDelay(((task_node_tick_get_32() * ((x) +1)) & 0x1f) + 1)
#endif

#define PRINT(x,y)	myPrint(x,y)

#ifdef CONFIG_NANOKERNEL
/* For the nanokernel. */
extern struct nano_sem forks[N_PHILOSOPHERS];
#else
/* For the microkernel. */
kmutex_t forks[] = { forkMutex0, forkMutex1, forkMutex2, forkMutex3, forkMutex4,
		forkMutex5 };
#endif

/*!
 * @brief Prints a philosopher's state.
 *
 * @param id A philosopher's ID.
 * @param str A string, either EATING or THINKING.
 *
 * @return N/A
 */
static void myPrint(int id,
		char *str
		) {
	PRINTF("\x1b[%d;%dHPhilosopher %d %s\n", id + 1, 1, id, str);
}

/*!
 * @brief Waits for a number of ticks to elapse.
 *
 * @param ticks Number of ticks to delay.
 *
 * @return N/A
 */
static void myDelay(int ticks
) {
#ifdef CONFIG_MICROKERNEL
	task_sleep (ticks);
#else
	struct nano_timer timer;

	nano_timer_init(&timer, (void *) 0);
	nano_fiber_timer_start(&timer, ticks);
	nano_fiber_timer_wait(&timer);
#endif
}

/*!
 * @brief Entry point to a philosopher's thread.
 *
 * @details This routine runs as a task in the microkernel environment
 * and as a fiber in the nanokernel environment.
 *
 * Actions:
 * 		-# Always takes the lowest fork first.
 * 		-# Prints out either Eating or Thinking.
 *
 * @return  Not applicable.
 */
void philEntry(void) {
#ifdef CONFIG_NANOKERNEL
	/*! Declares a fork for the nanokernel. */
	struct nano_sem *f1;

	/*! Declares a second fork for the nanokernel. */
	struct nano_sem *f2;
#else
	/*! Declares a fork for the microkernel. */
	kmutex_t f1;
	kmutex_t f2;
#endif
	/*! Declares the current philosopher's ID. */
	static int myId;

	/*! Declares an interrupt lock level.*/
	int pri = irq_lock();

	/*! Declares the next philosopher's ID. */
	int id = myId++;

	irq_unlock(pri);

	if ((id + 1) != N_PHILOSOPHERS) { /* A1 */
		f1 = FORK(id);
		f2 = FORK(id + 1);
	} else {
		f1 = FORK(0);
		f2 = FORK(id);
	}

	while (1) { /* A2 */
		TAKE(f1);
		TAKE(f2);

		PRINT(id, "EATING  ");
		RANDDELAY(id);

		GIVE(f2);
		GIVE(f1);

		PRINT(id, "THINKING");
		RANDDELAY(id);
	}
}
