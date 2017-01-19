/* test_fiber.c - test fiber functions */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
The module implements functions for the fiber that tests
semaphore signaling
 */

#include <zephyr.h>

#define N_TESTS 10 /* number of tests to run */
#define FIBER_PRIORITY 6
#define FIBER_STACK_SIZE 384

/* exports */
struct nano_sem fiberSem; /* semaphore that allows test control the fiber */

extern ksem_t simpleSem;
extern ksem_t semList[];

static char __stack fiberStack[FIBER_STACK_SIZE]; /* test fiber stack size */

/**
 *
 * @brief The test fiber entry function
 *
 * Fiber waits on the semaphore controlled by the test task
 * It signals the semaphore, the testing task waits for,
 * then it signals the semaphore for N_TASKS times, testing task
 * checks this number.
 * Then fiber signals each of the semaphores in the group. Test
 * task checks this.
 *
 * @return N/A
 */
static void testFiberEntry(void)
{
	int i;
	/* release semaphore test task is waiting for */
	nano_fiber_sem_take(&fiberSem, TICKS_UNLIMITED);
	fiber_sem_give(simpleSem);

	/* release the semaphore for N_TESTS times */
	nano_fiber_sem_take(&fiberSem, TICKS_UNLIMITED);
	for (i = 0; i < N_TESTS; i++) {
		fiber_sem_give(simpleSem);
	}

	/* signal each semaphore in the group */
	for (i = 0; semList[i] != ENDLIST; i++) {
		nano_fiber_sem_take(&fiberSem, TICKS_UNLIMITED);
		fiber_sem_give(semList[i]);
	}
}

/**
 *
 * @brief Initializes variables and starts the test fiber
 *
 * @return N/A
 */

void testFiberInit(void)
{
	nano_sem_init(&fiberSem);
	task_fiber_start(fiberStack, FIBER_STACK_SIZE, (nano_fiber_entry_t)testFiberEntry,
		    0, 0, FIBER_PRIORITY, 0);
}
