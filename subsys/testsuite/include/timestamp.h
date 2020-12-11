/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file macroses for measuring time in benchmarking tests
 *
 * This file contains the macroses for taking and converting time for
 * benchmarking tests.
 */

#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_
#include <zephyr.h>

#include <limits.h>
#if defined(__GNUC__)
#include <test_asm_inline_gcc.h>
#else
#include <test_asm_inline_other.h>
#endif


#define TICK_SYNCH()  k_sleep(K_TICKS(1))

#define OS_GET_TIME() k_cycle_get_32()

/* time necessary to read the time */
extern uint32_t tm_off;

static inline uint32_t TIME_STAMP_DELTA_GET(uint32_t ts)
{
	uint32_t t;

	/* serialize so OS_GET_TIME() is not reordered */
	timestamp_serialize();

	t = OS_GET_TIME();
	uint32_t res = (t >= ts) ? (t - ts) : (ULONG_MAX - ts + t);

	if (ts > 0) {
		res -= tm_off;
	}
	return res;
}

/*
 * Routine initializes the benchmark timing measurement
 * The function sets up the global variable tm_off
 */
static inline void bench_test_init(void)
{
	uint32_t t = OS_GET_TIME();

	tm_off = OS_GET_TIME() - t;
}


/* timestamp for checks */
static int64_t timestamp_check;

/*
 * Routines are invoked before and after the benchmark and check
 * if benchmarking code took less time than necessary for the
 * high precision timer register overflow.
 * Functions modify the timestamp_check global variable.
 */
static inline void bench_test_start(void)
{
	timestamp_check = 0;
	/* before reading time we synchronize to the start of the timer tick */
	TICK_SYNCH();
	timestamp_check = k_uptime_delta(&timestamp_check);
}


/* returns 0 if the completed within a second and -1 if not */
static inline int bench_test_end(void)
{
	timestamp_check = k_uptime_delta(&timestamp_check);

	/* Flag an error if the test ran for more than a second.
	 * (Note: Existing benchmarks have CONFIG_SYS_CLOCK_TICKS_PER_SEC=1 set,
	 * in such configs this check can be an indicator of whether
	 * timer tick interrupt overheads too are getting accounted towards
	 * benchmark time)
	 */
	if (timestamp_check >= MSEC_PER_SEC) {
		return -1;
	}
	return 0;
}

/*
 * Returns -1 if number of ticks cause high precision timer counter
 * overflow and 0 otherwise
 * Called after bench_test_end to see if we still can use timing
 * results or is it completely invalid
 */
static inline int high_timer_overflow(void)
{
	/* Check if the time elapsed in msec is sufficient to trigger an
	 *  overflow of the high precision timer
	 */
	if (timestamp_check >= (k_cyc_to_ns_floor64(UINT_MAX) /
				(NSEC_PER_USEC * USEC_PER_MSEC))) {
		return -1;
	}
	return 0;
}

#endif /* _TIMESTAMP_H_ */
