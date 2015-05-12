/* test_fiber.c - test fiber functions */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
The module implements functions for the fiber that tests
semaphore signaling
*/

/* includes */
#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <vxmicro.h>

/* defines */
#define N_TESTS 10 /* number of tests to run */
#define FIBER_PRIORITY 6
#define FIBER_STACK_SIZE 1024

/* exports */
struct nano_sem fiberSem; /* semaphore that allows test control the fiber */

/* locals */
extern ksem_t simpleSem;
extern ksem_t semList[];

static char fiberStack[FIBER_STACK_SIZE]; /* test fiber stack size */

/* array of command packets used by test fiber to signal semaphores */
static CMD_PKT_SET_INSTANCE(cmdPktSet, N_TESTS + 1)

/*******************************************************************************
*
* testFiberEntry - the test fiber entry function
*
* Fiber waits on the semaphore controlled by the test task
* It signals the semaphore, the testing task waits for,
* then it signals the semaphore for N_TASKS times, testing task
* checks this number.
* Then fiber signals each of the semaphores in the group. Test
* task checks this.
*
* RETURNS: N/A
*/
static void testFiberEntry(void)
	{
	int i;
    /* release semaphore test task is waiting for */
	nano_fiber_sem_take_wait(&fiberSem);
	fiber_sem_give(simpleSem, &CMD_PKT_SET(cmdPktSet));

    /* release the semaphore for N_TESTS times */
	nano_fiber_sem_take_wait(&fiberSem);
	for (i = 0; i < N_TESTS; i++) {
	fiber_sem_give(simpleSem, &CMD_PKT_SET(cmdPktSet));
	}

    /* signal each semaphore in the group */
	for (i = 0; semList[i] != ENDLIST; i++) {
	nano_fiber_sem_take_wait(&fiberSem);
	fiber_sem_give(semList[i], &CMD_PKT_SET(cmdPktSet));
	}
	}

/*******************************************************************************
*
* testFiberInit - initializes variables and starts the test fiber
*
* RETURNS: N/A
*/

void testFiberInit(void)
	{
	nano_sem_init(&fiberSem);
	task_fiber_start(fiberStack, FIBER_STACK_SIZE, (nano_fiber_entry_t)testFiberEntry,
		    0, 0, FIBER_PRIORITY, 0);
	}
