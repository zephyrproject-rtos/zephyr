/* int_latency_bench.c - interrupt latency benchmark support */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toolchain.h"
#include "sections.h"
#include <zephyr/types.h>	    /* u32_t */
#include <limits.h>	    /* ULONG_MAX */
#include <misc/printk.h> /* printk */
#include <sys_clock.h>
#include <drivers/system_timer.h>

#define NB_CACHE_WARMING_DRY_RUN 7

/*
 * Timestamp corresponding to when interrupt were turned off.
 * A value of zero indicated interrupt are not currently locked.
 */
static u32_t int_locked_timestamp;

/* stats tracking the minimum and maximum time when interrupts were locked */
static u32_t int_locked_latency_min = ULONG_MAX;
static u32_t int_locked_latency_max;

/* overhead added to intLock/intUnlock by this latency benchmark */
static u32_t initial_start_delay;
static u32_t nesting_delay;
static u32_t stop_delay;

/* counter tracking intLock/intUnlock calls once interrupt are locked */
static u32_t int_lock_unlock_nest;

/* indicate if the interrupt latency benchamrk is ready to be used */
static u32_t int_latency_bench_ready;

/* min amount of time it takes from HW interrupt generation to 'C' handler */
u32_t _hw_irq_to_c_handler_latency = ULONG_MAX;

/**
 *
 * @brief Start tracking time spent with interrupts locked
 *
 * calls to lock interrupt can nest, so this routine can be called numerous
 * times before interrupt are unlocked
 *
 * @return N/A
 *
 */
void _int_latency_start(void)
{
	/* when interrupts are not already locked, take time stamp */
	if (!int_locked_timestamp && int_latency_bench_ready) {
		int_locked_timestamp = k_cycle_get_32();
		int_lock_unlock_nest = 0U;
	}
	int_lock_unlock_nest++;
}

/**
 *
 * @brief Stop accumulating time spent for when interrupts are locked
 *
 * This is only call once when the interrupt are being reenabled
 *
 * @return N/A
 *
 */
void _int_latency_stop(void)
{
	u32_t delta;
	u32_t delayOverhead;
	u32_t currentTime = k_cycle_get_32();

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
			 ((int_lock_unlock_nest - 1) * nesting_delay) + stop_delay);
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
		int_locked_timestamp = 0U;
	}
}

/**
 *
 * @brief Initialize interrupt latency benchmark
 *
 * @return N/A
 *
 */
void int_latency_init(void)
{
	u32_t timeToReadTime;
	u32_t cacheWarming = NB_CACHE_WARMING_DRY_RUN;

	int_latency_bench_ready = 1U;

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
		timeToReadTime = k_cycle_get_32();
		timeToReadTime = k_cycle_get_32() - timeToReadTime;

		/* measure time to call intLatencyStart() and intLatencyStop
		 * takes
		 */
		initial_start_delay = k_cycle_get_32();
		_int_latency_start();
		initial_start_delay =
			k_cycle_get_32() - initial_start_delay - timeToReadTime;

		nesting_delay = k_cycle_get_32();
		_int_latency_start();
		nesting_delay = k_cycle_get_32() - nesting_delay - timeToReadTime;

		stop_delay = k_cycle_get_32();
		_int_latency_stop();
		stop_delay = k_cycle_get_32() - stop_delay - timeToReadTime;

		/* re-initialize globals to default values */
		int_locked_latency_min = ULONG_MAX;
		int_locked_latency_max = 0U;

		cacheWarming--;
	}
}

/**
 *
 * @brief Dumps interrupt latency values
 *
 * The interrupt latency value measures
 *
 * @return N/A
 *
 */
void int_latency_show(void)
{
	u32_t intHandlerLatency = 0U;

	if (int_latency_bench_ready == 0) {
		printk("error: int_latency_init() has not been invoked\n");
		return;
	}

	if (int_locked_latency_min != ULONG_MAX) {
		if (_hw_irq_to_c_handler_latency == ULONG_MAX) {
			intHandlerLatency = 0U;
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
	int_locked_latency_max = 0U;
}
