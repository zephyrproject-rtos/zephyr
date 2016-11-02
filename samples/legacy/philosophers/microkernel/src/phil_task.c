/* phil_task.c - dining philosophers */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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

#include <zephyr.h>
#include "phil.h"

#define DEMO_DESCRIPTION  \
	"\x1b[2J\x1b[15;1H"   \
	"Demo Description\n"  \
	"----------------\n"  \
	"An implementation of a solution to the Dining Philosophers problem\n" \
	"(a classic multi-thread synchronization problem).  This particular\n" \
	"implementation demonstrates the usage of multiple (6) %s\n"           \
	"of differing priorities and the %s semaphores and timers."

#ifdef NANO_APIS_ONLY
#define STSIZE 1024

/* externs */

extern void philEntry(void);

char __stack philStack[N_PHILOSOPHERS][STSIZE];
struct nano_sem forks[N_PHILOSOPHERS];
#endif

#ifdef NANO_APIS_ONLY
/**
 *
 * @brief Nanokernel entry point
 *
 * @return does not return
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
/**
 *
 * @brief Routine to start dining philosopher demo
 *
 * @return does not return
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
