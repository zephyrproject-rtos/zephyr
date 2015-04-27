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
static uint32_t int_locked_timestamp = 0;

/* stats tracking the minimum and maximum time when interrupts were locked */
static uint32_t int_locked_latency_min = ULONG_MAX;
static uint32_t int_locked_latency_max = 0;

/* overhead added to intLock/intUnlock by this latency benchmark */
static uint32_t initial_start_delay = 0;
static uint32_t nesting_delay = 0;
static uint32_t stop_delay = 0;

/* counter tracking intLock/intUnlock calls once interrupt are locked */
static uint32_t intLockUnlockNest = 0;

/* indicate if the interrupt latency benchamrk is ready to be used */
static uint32_t int_latency_bench_ready = 0;

/* globals */

/* min amount of time it takes from HW interrupt generation to 'C' handler */
uint32_t _hw_irq_to_c_handler_latency = ULONG_MAX;

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
	if (!int_locked_timestamp && int_latency_bench_ready) {
		int_locked_timestamp = timer_read();
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
	if (int_locked_timestamp) {
		/*
		 * time spent with interrupt lock is:
		 * (current time - time when interrupt got disabled first) -
		 * (delay when invoking start + number nested calls to intLock *
		 * time it takes to call intLatencyStart + intLatencyStop)
		 */
		delta = (currentTime - int_locked_timestamp);

		/*
		 * Substract overhead introduce by the int latency benchmark
		 * only if
		 * it is bigger than delta.  It can be possible sometimes for
		 * delta to
		 * be smaller than the estimated overhead.
		 */
		delayOverhead =
			(initial_start_delay +
			 ((intLockUnlockNest - 1) * nesting_delay) + stop_delay);
		if (delta >= delayOverhead)
			delta -= delayOverhead;

		/* update max */
		if (delta > int_locked_latency_max)
			int_locked_latency_max = delta;

		/* update min */
		if (delta < int_locked_latency_min)
			int_locked_latency_min = delta;

		/* interrupts are now enabled, get ready for next interrupt lock
		 */
		int_locked_timestamp = 0;
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

	int_latency_bench_ready = 1;

	/*
	 * measuring delay introduced by the interrupt latency benchmark few
	 * times to ensure we get the best possible values. The overhead of
	 * invoking the latency can changes runtime (i.e. cache hit or miss)
	 * but an estimated overhead is used to adjust Max interrupt latency.
	 * The overhead introduced by benchmark is composed of three values:
	 * initial_start_delay, nesting_delay, stop_delay.
	 */
	while (cacheWarming) {
		/* measure how much time it takes to read time */
		timeToReadTime = timer_read();
		timeToReadTime = timer_read() - timeToReadTime;

		/* measure time to call intLatencyStart() and intLatencyStop
		 * takes */
		initial_start_delay = timer_read();
		_int_latency_start();
		initial_start_delay =
			timer_read() - initial_start_delay - timeToReadTime;

		nesting_delay = timer_read();
		_int_latency_start();
		nesting_delay = timer_read() - nesting_delay - timeToReadTime;

		stop_delay = timer_read();
		_int_latency_stop();
		stop_delay = timer_read() - stop_delay - timeToReadTime;

		/* re-initialize globals to default values */
		int_locked_latency_min = ULONG_MAX;
		int_locked_latency_max = 0;

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

	if (!int_latency_bench_ready) {
		printk("error: int_latency_init() has not been invoked\n");
		return;
	}

	if (int_locked_latency_min != ULONG_MAX) {
		if (_hw_irq_to_c_handler_latency == ULONG_MAX) {
			intHandlerLatency = 0;
			printk(" Min latency from hw interrupt up to 'C' int. "
			       "handler: "
			       "not measured\n");
		} else {
			intHandlerLatency = _hw_irq_to_c_handler_latency;
			printk(" Min latency from hw interrupt up to 'C' int. "
			       "handler:"
			       " %d tcs = %d nsec\n",
			       intHandlerLatency,
			       SYS_CLOCK_HW_CYCLES_TO_NS(intHandlerLatency));
		}

		printk(" Max interrupt latency (includes hw int. to 'C' "
		       "handler):"
		       " %d tcs = %d nsec\n",
		       int_locked_latency_max + intHandlerLatency,
		       SYS_CLOCK_HW_CYCLES_TO_NS(int_locked_latency_max + intHandlerLatency));

		printk(" Overhead substracted from Max int. latency:\n"
		       "  for int. lock           : %d tcs = %d nsec\n"
		       "  each time int. lock nest: %d tcs = %d nsec\n"
		       "  for int. unlocked       : %d tcs = %d nsec\n",
		       initial_start_delay,
		       SYS_CLOCK_HW_CYCLES_TO_NS(initial_start_delay),
		       nesting_delay,
		       SYS_CLOCK_HW_CYCLES_TO_NS(nesting_delay),
		       stop_delay,
		       SYS_CLOCK_HW_CYCLES_TO_NS(stop_delay));
	} else {
		printk("interrupts were not locked and unlocked yet\n");
	}
	/*
	 * Lets start with new values so that one extra long path executed
	 * with interrupt disabled hide smaller paths with interrupt
	 * disabled.
	 */
	int_locked_latency_min = ULONG_MAX;
	int_locked_latency_max = 0;
}

#endif /* CONFIG_INT_LATENCY_BENCHMARK */
