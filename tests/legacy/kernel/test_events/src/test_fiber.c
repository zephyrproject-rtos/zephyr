/* test_fiber.c - test fiber functions */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
The module implements functions for the fiber that tests
event signaling
 */
#include <zephyr.h>

#define N_TESTS 10 /* number of tests to run */
#define FIBER_PRIORITY 6
#define FIBER_STACK_SIZE 1024

/* exports */
struct nano_sem fiberSem; /* semaphore that allows test control the fiber */

static char __stack fiberStack[FIBER_STACK_SIZE]; /* test fiber stack size */

/**
 *
 * @brief The test fiber entry function
 *
 * Fiber waits on the semaphore controlled by the test task
 * It signals the event for the eventWaitTest() function
 * in single and cycle test, for eventTimeoutTest()
 *
 * @return N/A
 */
static void testFiberEntry(void)
{
	/* signal event for eventWaitTest() */
	/* single test */
	nano_fiber_sem_take(&fiberSem, TICKS_UNLIMITED);
	fiber_event_send(EVENT_ID);
	/* test in cycle */
	nano_fiber_sem_take(&fiberSem, TICKS_UNLIMITED);
	fiber_event_send(EVENT_ID);

	/* signal event for eventTimeoutTest() */
	nano_fiber_sem_take(&fiberSem, TICKS_UNLIMITED);
	fiber_event_send(EVENT_ID);

	/*
	 * Signal two events for fiberEventSignalTest ().
	 * It has to detect only one
	 */
	nano_fiber_sem_take(&fiberSem, TICKS_UNLIMITED);
	fiber_event_send(EVENT_ID);
	fiber_event_send(EVENT_ID);
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
