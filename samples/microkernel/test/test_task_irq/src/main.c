/* main.c - main testing module */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
 * DESCRIPTION
 * This file contains the main testing module that invokes all the tests.
 */

#include <tc_util.h>
#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <vxmicro.h>

/* One of the task IRQ objects will not be allocated */
#define NUM_TASK_IRQS   CONFIG_MAX_NUM_TASK_IRQS - 1

#define NUM_TEST_TASKS	3	/* # of test tasks to monitor */

/* # ticks to wait for test completion */
#define TIMEOUT	(60 * sys_clock_ticks_per_sec)

/* locals */

static ksem_t resultSems[] = { SEM_TASKDONE, SEM_TASKFAIL, ENDLIST };
static ksem_t rdySem = SEM_RDY;

/*******************************************************************************
*
* taskAMain - entry point for taskA
*
* This routine signals "task done" or "task fail", based on the return code of
* taskA.
*
* RETURNS: N/A
*/

void taskAMain(void)
	{
	extern int taskA(ksem_t semRdy);
	task_sem_give (resultSems[taskA(rdySem)]);
	}

/*******************************************************************************
*
* taskBMain - entry point for taskB
*
* This routine signals "task done" or "task fail", based on the return code of
* taskB.
*
* RETURNS: N/A
*/

void taskBMain(void)
	{
	extern int taskB(ksem_t semRdy);
	task_sem_give (resultSems[taskB(rdySem)]);
	}

/*******************************************************************************
*
* registerWait - wait for devices to be registered and generate SW ints
*
* This routine waits for the tasks to indicate the IRQ objects are allocated and
* then generates SW interrupts for all IRQs. Signals "task done" if all task
* indicated the IRQs are allocated or signals "task fail"if not.
*
* RETURNS: N/A
*/
void registerWait (void)
	{
	extern void raiseInt(uint8_t id);
	int tasksDone;
	int irq_obj;

    /* Wait for the 2 tasks to finish registering their IRQ objects*/

	for (tasksDone = 0; tasksDone < NUM_TEST_TASKS - 1; tasksDone++) {
		if (task_sem_take_wait_timeout (SEM_RDY, TIMEOUT) != RC_OK) {
		    TC_ERROR ("Monitor task timed out\n");
		    task_sem_give (resultSems[TC_FAIL]);
		    return;
		    }
		}

	TC_PRINT ("Generating interrupts for all allocated IRQ objects...\n");
	for (irq_obj = 0; irq_obj < NUM_TASK_IRQS; irq_obj++) {
	if (task_irq_object[irq_obj].irq != INVALID_VECTOR)
	    raiseInt((uint8_t)task_irq_object[irq_obj].vector);
		}

	task_sem_give (resultSems[TC_PASS]);
	}

/*******************************************************************************
*
* MonitorTaskEntry - entry point for MonitorTask
*
* This routine keeps tabs on the progress of the tasks doing the actual testing
* and generates the final test case summary message.
*
* RETURNS: N/A
*/

void MonitorTaskEntry (void)
	{
	extern struct task_irq_info task_irq_object[NUM_TASK_IRQS];
	ksem_t result;
	int tasksDone;

	PRINT_DATA ("Starting task level interrupt handling tests\n");
	PRINT_LINE;

    /*
     * the various test tasks start executing automatically;
     * wait for all tasks to complete or a failure to occur,
     * then issue the appropriate test case summary message
     */

	for (tasksDone = 0; tasksDone < NUM_TEST_TASKS; tasksDone++) {
		result = task_sem_group_take_wait_timeout (resultSems, TIMEOUT);
		if (result != resultSems[TC_PASS]) {
		    if (result != resultSems[TC_FAIL]) {
		        TC_ERROR ("Monitor task timed out\n");
		        }
		    TC_END_RESULT (TC_FAIL);
		    TC_END_REPORT (TC_FAIL);
		    return;
		    }
		}

	TC_END_RESULT (TC_PASS);
	TC_END_REPORT (TC_PASS);
	}
