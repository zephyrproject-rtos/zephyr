/** @file
 * @brief An implementation of a solution to the dining philosophers problem
 *  for both the nano- and microkernel.
 *
 * This particular implementation uses 6 fibers or tasks of
 * different priority, semaphores and timers. The implementation demostrates
 * fibers and semaphores in the nanokernel and tasks and timers in the
 * microkernel.
 */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

#ifdef CONFIG_NANOKERNEL
#include <nanokernel.h>
#include "phil.h"
#else
#include <zephyr.h>
#include "phil.h"
#endif

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

extern void philEntry (void); //!< External function.

char philStack[N_PHILOSOPHERS][STSIZE];//!< Declares a global stack of size 1024.
struct nano_sem forks[N_PHILOSOPHERS];//!< Declares global semaphore forks for the number of philosophers.
#endif  /*  CONFIG_NANOKERNEL */

#ifdef CONFIG_NANOKERNEL

/**
 * @brief The nanokernel entry point.
 *
 * Actions:
 * 		-# Starts one fiber per philosopher.
 * 		-# Waits forever.
 * @return Does not return.
 */
int main (void)
{
	int i;

	PRINTF (DEMO_DESCRIPTION, "fibers", "nanokernel");

	for (i = 0; i < N_PHILOSOPHERS; i++)
	{
		nano_sem_init (&forks[i]);
		nano_task_sem_give (&forks[i]);
	}

	/* A1 */
	for (i = 0; i < N_PHILOSOPHERS; i++)
	task_fiber_start (&philStack[i][0], STSIZE,
			(nano_fiber_entry_t) philEntry, 0, 0, 6, 0);

	/* A2 */
	while (1)
	{
		extern void nano_cpu_idle (void);
		nano_cpu_idle ();
	}
}

#else

/**
 * @brief Starts the dining philosophers demo of the microkernel.
 *
 * This function starts the dining philosophers demo and
 * then waits forever.
 * @return Does not return.
 */
void philDemo(void) {
	PRINTF(DEMO_DESCRIPTION, "tasks", "microkernel");

	task_group_start(PHI);

	while (1) {
		task_sleep(10000);
	}
}
#endif
