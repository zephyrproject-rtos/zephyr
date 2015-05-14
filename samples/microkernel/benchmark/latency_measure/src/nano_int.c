/* nano_int.c - measure the time from task to ISR */

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
 * This file contains test that measures time to switch time from a fiber
 * to the interrupt handler when an interrupt is generated.
 */

#include "timestamp.h"
#include "utils.h"

#include <nanokernel/cpu.h>

#define STACKSIZE 2000

/* stack used by the fiber that generates the interrupt */
static char fiberStack[STACKSIZE];

static uint32_t timestamp;

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

	timestamp = TIME_STAMP_DELTA_GET(timestamp);
}

/*******************************************************************************
*
* fiberInt - interrupt preparation fiber
*
* Fiber makes all the test preparations: registers the interrupt handler,
* gets the first timestamp and invokes the software interrupt.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void fiberInt(void)
{
	initSwInterrupt(latencyTestIsr);
	timestamp = TIME_STAMP_DELTA_GET(0);
	raiseIntFunc();
}

/*******************************************************************************
 *
 * nanoIntLatency - the test main function
 *
 * RETURNS: 0 on success
 *
 * \NOMANUAL
 */

int nanoIntLatency(void)
{
	PRINT_FORMAT(" 1- Measure time to switch from fiber to ISR execution");
	TICK_SYNCH();
	task_fiber_start(&fiberStack[0], STACKSIZE,
					 (nano_fiber_entry_t) fiberInt, 0, 0, 6, 0);
	PRINT_FORMAT(" switching time is %lu tcs = %lu nsec",
				 timestamp, SYS_CLOCK_HW_CYCLES_TO_NS(timestamp));
	return 0;
}
