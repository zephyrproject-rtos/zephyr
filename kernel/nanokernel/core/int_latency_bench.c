/* int_latency_bench.c - interrupt latency benchmark support */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
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

/* includes */

#ifdef CONFIG_INT_LATENCY_BENCHMARK

#include "toolchain.h"
#include "sections.h"
#include <stdint.h>	    /* uint32_t */
#include <limits.h>	    /* ULONG_MAX */
#include <misc/printk.h> /* printk */
#include <clock_vars.h>
#include <drivers/system_timer.h>

/* defines */

#define NB_CACHE_WARMING_DRY_RUN 7

/* locals */

/*
 * Timestamp corresponding to when interrupt were turned off.
 * A value of zero indicated interrupt are not currently locked.
 */
static uint32_t intLockedTimestamp = 0;

/* stats tracking the minimum and maximum time when interrupts were locked */
static uint32_t intLockingLatencyMin = ULONG_MAX;
static uint32_t intLockingLatencyMax = 0;

/* overhead added to intLock/intUnlock by this latency benchmark */
static uint32_t initialStartDelay = 0;
static uint32_t nestingDelay = 0;
static uint32_t stopDelay = 0;

/* counter tracking intLock/intUnlock calls once interrupt are locked */
static uint32_t intLockUnlockNest = 0;

/* indicate if the interrupt latency benchamrk is ready to be used */
static uint32_t intLatencyBenchRdy = 0;

/* globals */

/* min amount of time it takes from HW interrupt generation to 'C' handler */
uint32_t _HwIntToCHandlerLatency = ULONG_MAX;

/*******************************************************************************
*
* intLatencyStart - start tracking time spent with interrupts locked
*
* calls to lock interrupt can nest, so this routine can be called numerous
* times before interrupt are unlocked
*
* RETURNS: N/A
*
*/

void _int_latency_start(void)
{
	/* when interrupts are not already locked, take time stamp */
	if (!intLockedTimestamp && intLatencyBenchRdy) {
		intLockedTimestamp = timer_read();
		intLockUnlockNest = 0;
	}
	intLockUnlockNest++;
}

/*******************************************************************************
*
* intLatencyStop - stop accumulating time spent for when interrupts are locked
*
* This is only call once when the interrupt are being reenabled
*
* RETURNS: N/A
*
*/

void _int_latency_stop(void)
{
	uint32_t delta;
	uint32_t delayOverhead;
	uint32_t currentTime = timer_read();

	/* ensured intLatencyStart() was invoked first */
	if (intLockedTimestamp) {
		/*
		 * time spent with interrupt lock is:
		 * (current time - time when interrupt got disabled first) -
		 * (delay when invoking start + number nested calls to intLock *
		 * time it takes to call intLatencyStart + intLatencyStop)
		 */
		delta = (currentTime - intLockedTimestamp);

		/*
		 * Substract overhead introduce by the int latency benchmark
		 * only if
		 * it is bigger than delta.  It can be possible sometimes for
		 * delta to
		 * be smaller than the estimated overhead.
		 */
		delayOverhead =
			(initialStartDelay +
			 ((intLockUnlockNest - 1) * nestingDelay) + stopDelay);
		if (delta >= delayOverhead)
			delta -= delayOverhead;

		/* update max */
		if (delta > intLockingLatencyMax)
			intLockingLatencyMax = delta;

		/* update min */
		if (delta < intLockingLatencyMin)
			intLockingLatencyMin = delta;

		/* interrupts are now enabled, get ready for next interrupt lock
		 */
		intLockedTimestamp = 0;
	}
}

/*******************************************************************************
*
* int_latency_init - initialize interrupt latency benchmark
*
* RETURNS: N/A
*
*/

void int_latency_init(void)
{
	uint32_t timeToReadTime;
	uint32_t cacheWarming = NB_CACHE_WARMING_DRY_RUN;

	intLatencyBenchRdy = 1;

	/*
	 * measuring delay introduced by the interrupt latency benchmark few
	 * times to ensure we get the best possible values. The overhead of
	 * invoking the latency can changes runtime (i.e. cache hit or miss)
	 * but an estimated overhead is used to adjust Max interrupt latency.
	 * The overhead introduced by benchmark is composed of three values:
	 * initialStartDelay, nestingDelay, stopDelay.
	 */
	while (cacheWarming) {
		/* measure how much time it takes to read time */
		timeToReadTime = timer_read();
		timeToReadTime = timer_read() - timeToReadTime;

		/* measure time to call intLatencyStart() and intLatencyStop
		 * takes */
		initialStartDelay = timer_read();
		_int_latency_start();
		initialStartDelay =
			timer_read() - initialStartDelay - timeToReadTime;

		nestingDelay = timer_read();
		_int_latency_start();
		nestingDelay = timer_read() - nestingDelay - timeToReadTime;

		stopDelay = timer_read();
		_int_latency_stop();
		stopDelay = timer_read() - stopDelay - timeToReadTime;

		/* re-initialize globals to default values */
		intLockingLatencyMin = ULONG_MAX;
		intLockingLatencyMax = 0;

		cacheWarming--;
	}
}

/*******************************************************************************
*
* int_latency_show - dumps interrupt latency values
*
* The interrupt latency value measures
*
* RETURNS: N/A
*
*/

void int_latency_show(void)
{
	uint32_t intHandlerLatency = 0;

	if (!intLatencyBenchRdy) {
		printk("error: int_latency_init() has not been invoked\n");
		return;
	}

	if (intLockingLatencyMin != ULONG_MAX) {
		if (_HwIntToCHandlerLatency == ULONG_MAX) {
			intHandlerLatency = 0;
			printk(" Min latency from hw interrupt up to 'C' int. "
			       "handler: "
			       "not measured\n");
		} else {
			intHandlerLatency = _HwIntToCHandlerLatency;
			printk(" Min latency from hw interrupt up to 'C' int. "
			       "handler:"
			       " %d tcs = %d nsec\n",
			       intHandlerLatency,
			       SYS_CLOCK_HW_CYCLES_TO_NS(intHandlerLatency));
		}

		printk(" Max interrupt latency (includes hw int. to 'C' "
		       "handler):"
		       " %d tcs = %d nsec\n",
		       intLockingLatencyMax + intHandlerLatency,
		       SYS_CLOCK_HW_CYCLES_TO_NS(intLockingLatencyMax + intHandlerLatency));

		printk(" Overhead substracted from Max int. latency:\n"
		       "  for int. lock           : %d tcs = %d nsec\n"
		       "  each time int. lock nest: %d tcs = %d nsec\n"
		       "  for int. unlocked       : %d tcs = %d nsec\n",
		       initialStartDelay,
		       SYS_CLOCK_HW_CYCLES_TO_NS(initialStartDelay),
		       nestingDelay,
		       SYS_CLOCK_HW_CYCLES_TO_NS(nestingDelay),
		       stopDelay,
		       SYS_CLOCK_HW_CYCLES_TO_NS(stopDelay));
	} else {
		printk("interrupts were not locked and unlocked yet\n");
	}
	/*
	 * Lets start with new values so that one extra long path executed
	 * with interrupt disabled hide smaller paths with interrupt
	 * disabled.
	 */
	intLockingLatencyMin = ULONG_MAX;
	intLockingLatencyMax = 0;
}

#endif /* CONFIG_INT_LATENCY_BENCHMARK */
