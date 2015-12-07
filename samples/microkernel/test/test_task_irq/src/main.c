/* main.c - main testing module */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
 * DESCRIPTION
 * This file contains the main testing module that invokes all the tests.
 */

#include <zephyr.h>
#include <tc_util.h>

/* One of the task IRQ objects will not be allocated */
#define NUM_TASK_IRQS   CONFIG_MAX_NUM_TASK_IRQS - 1

#define NUM_TEST_TASKS	3	/* # of test tasks to monitor */

/* # ticks to wait for test completion */
#define TIMEOUT	(60 * sys_clock_ticks_per_sec)

static ksem_t resultSems[] = { SEM_TASKDONE, SEM_TASKFAIL, ENDLIST };
static ksem_t rdySem = SEM_RDY;

#define NUM_OBJECTS 4
extern uint32_t irq_vectors[NUM_OBJECTS];

/**
 *
 * @brief Entry point for taskA
 *
 * This routine signals "task done" or "task fail", based on the return code of
 * taskA.
 *
 * @return N/A
 */

void taskAMain(void)
{
	extern int taskA(ksem_t semRdy);
	task_sem_give(resultSems[taskA(rdySem)]);
}

/**
 *
 * @brief Entry point for taskB
 *
 * This routine signals "task done" or "task fail", based on the return code of
 * taskB.
 *
 * @return N/A
 */

void taskBMain(void)
{
	extern int taskB(ksem_t semRdy);
	task_sem_give(resultSems[taskB(rdySem)]);
}

/**
 *
 * @brief Wait for devices to be registered and generate SW ints
 *
 * This routine waits for the tasks to indicate the IRQ objects are allocated and
 * then generates SW interrupts for all IRQs. Signals "task done" if all task
 * indicated the IRQs are allocated or signals "task fail"if not.
 *
 * @return N/A
 */
void registerWait(void)
{
	extern void raiseInt(uint8_t id);
	int tasksDone;
	int irq_obj;

	/* Wait for the 2 tasks to finish registering their IRQ objects*/

	for (tasksDone = 0; tasksDone < NUM_TEST_TASKS - 1; tasksDone++) {
		if (task_sem_take(SEM_RDY, TIMEOUT) != RC_OK) {
			TC_ERROR("Monitor task timed out\n");
			task_sem_give(resultSems[TC_FAIL]);
			return;
		}
	}

	TC_PRINT("Generating interrupts for all allocated IRQ objects...\n");
	for (irq_obj = 0; irq_obj < NUM_OBJECTS; irq_obj++) {
		if (irq_vectors[irq_obj] != INVALID_VECTOR) {
			raiseInt((uint8_t)irq_vectors[irq_obj]);
		}
	}

	task_sem_give(resultSems[TC_PASS]);
}

/**
 *
 * @brief Entry point for MonitorTask
 *
 * This routine keeps tabs on the progress of the tasks doing the actual testing
 * and generates the final test case summary message.
 *
 * @return N/A
 */

void MonitorTaskEntry(void)
{
	ksem_t result;
	int tasksDone;

	PRINT_DATA("Starting task level interrupt handling tests\n");
	PRINT_LINE;

	/*
	 * the various test tasks start executing automatically;
	 * wait for all tasks to complete or a failure to occur,
	 * then issue the appropriate test case summary message
	 */

	for (tasksDone = 0; tasksDone < NUM_TEST_TASKS; tasksDone++) {
		result = task_sem_group_take_wait_timeout(resultSems, TIMEOUT);
		if (result != resultSems[TC_PASS]) {
			if (result != resultSems[TC_FAIL]) {
				TC_ERROR("Monitor task timed out\n");
			}
			TC_END_RESULT(TC_FAIL);
			TC_END_REPORT(TC_FAIL);
			return;
		}
	}

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}
