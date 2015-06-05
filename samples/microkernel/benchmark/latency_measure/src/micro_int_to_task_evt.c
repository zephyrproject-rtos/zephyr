/* micro_int_to_task_evt.c - measure time from ISR to a rescheduled task */

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

/*
 * DESCRIPTION
 * This file contains test that measures time to switch from an interrupt
 * handler to executing a task after rescheduling. In other words, execution
 * after interrupt handler resume in a different task than the one which got
 * interrupted.
 */

#ifdef CONFIG_MICROKERNEL
#include <zephyr.h>

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>

static uint32_t timestamp = 0;

/*******************************************************************************
*
* latencyTestIsr - test ISR used to measure best case interrupt latency
*
* The interrupt handler gets the second timestamp.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void latencyTestIsr(void *unused)
{
	ARG_UNUSED(unused);

	isr_event_send(EVENT0);
	timestamp = TIME_STAMP_DELTA_GET(0);
}

/*******************************************************************************
 *
 * microInt - software interrupt generating task
 *
 * Lower priority task that, when starts, wats for the semaphore. When gets
 * released by the main task, sets up the interrupt handler and generates the
 * software interrupt
 *
 * RETURNS: 0 on success
 *
 * \NOMANUAL
 */

void microInt(void)
{
	task_sem_take_wait(INTSEMA);
	setSwInterrupt(latencyTestIsr);
	raiseIntFunc();
	task_suspend(task_id_get());
}

/*******************************************************************************
 *
 * microIntToTaskEvt - the test main function
 *
 * RETURNS: 0 on success
 *
 * \NOMANUAL
 */

int microIntToTaskEvt(void)
{
	PRINT_FORMAT(" 2- Measure time from ISR to executing a different task"
				 " (rescheduled)");
	TICK_SYNCH();
	task_sem_give(INTSEMA);
	task_event_recv_wait(EVENT0);
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	PRINT_FORMAT(" switch time is %lu tcs = %lu nsec",
				 timestamp, SYS_CLOCK_HW_CYCLES_TO_NS(timestamp));
	return 0;
}

#endif /* CONFIG_MICROKERNEL */
