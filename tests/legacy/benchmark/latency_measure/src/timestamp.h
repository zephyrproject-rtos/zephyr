/* timestamp.h - macroses for measuring time in benchmarking tests */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
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

typedef int64_t TICK_TYPE;

#define TICK_SYNCH() task_sleep(1)

#define TICK_GET(x) ((TICK_TYPE) sys_tick_delta(x))
#define OS_GET_TIME() sys_cycle_get_32()

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

/* number of ticks before timer overflows */
#define BENCH_MAX_TICKS (sys_clock_ticks_per_sec - 1)

/* tickstamp used for timer counter overflow check */
static TICK_TYPE tCheck;

/*
 * Routines are invoked before and after the benchmark and check
 * if penchmarking code took less time than necessary for the
 * high precision timer register overflow.
 * Functions modify the tCheck global variable.
 */
static inline void bench_test_start(void)
{
	tCheck = 0;
	/* before reading time we synchronize to the start of the timer tick */
	TICK_SYNCH();
	tCheck = TICK_GET(&tCheck);
}


/* returns 0 if the number of ticks is valid and -1 if not */
static inline int bench_test_end(void)
{
	tCheck = TICK_GET(&tCheck);
	if (tCheck > BENCH_MAX_TICKS) {
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
	if (tCheck >= (UINT_MAX / sys_clock_hw_cycles_per_tick)) {
		return -1;
	}
	return 0;
}

#endif /* _TIMESTAMP_H_ */
