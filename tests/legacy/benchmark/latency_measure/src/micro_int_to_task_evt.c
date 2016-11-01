/* micro_int_to_task_evt.c - measure time from ISR to a rescheduled task */

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

/*
 * DESCRIPTION
 * This file contains test that measures time to switch from an interrupt
 * handler to executing a task after rescheduling. In other words, execution
 * after interrupt handler resume in a different task than the one which got
 * interrupted.
 */

#include <zephyr.h>
#include <irq_offload.h>

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>

static uint32_t timestamp;

/**
 *
 * @brief Test ISR used to measure best case interrupt latency
 *
 * The interrupt handler gets the second timestamp.
 *
 * @return N/A
 */
static void latencyTestIsr(void *unused)
{
	ARG_UNUSED(unused);

	isr_event_send(EVENT0);
	timestamp = TIME_STAMP_DELTA_GET(0);
}

/**
 *
 * @brief Software interrupt generating task
 *
 * Lower priority task that, when starts, waits for a semaphore. When gets
 * it, released by the main task, sets up the interrupt handler and generates
 * the software interrupt
 *
 * @return 0 on success
 */
void microInt(void)
{
	task_sem_take(INTSEMA, TICKS_UNLIMITED);
	irq_offload(latencyTestIsr, NULL);
	task_suspend(task_id_get());
}

/**
 *
 * @brief The test main function
 *
 * @return 0 on success
 */
int microIntToTaskEvt(void)
{
	PRINT_FORMAT(" 2 - Measure time from ISR to executing a different task"
		     " (rescheduled)");
	TICK_SYNCH();
	task_sem_give(INTSEMA);
	task_event_recv(EVENT0, TICKS_UNLIMITED);
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	PRINT_FORMAT(" switch time is %lu tcs = %lu nsec",
		     timestamp, SYS_CLOCK_HW_CYCLES_TO_NS(timestamp));
	return 0;
}
