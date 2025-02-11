/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_LIB_CLIB_SRC_TEST_THRD_H_
#define TESTS_LIB_CLIB_SRC_TEST_THRD_H_

#include <stdint.h>

#include <zephyr/posix/time.h>
#include <zephyr/sys_clock.h>

/* arbitrary magic numbers used for testing */
#define BIOS_FOOD     0xb105f00d
#define FORTY_TWO     42
#define SEVENTY_THREE 73
#define DONT_CARE     0x370ca2e5

static inline void timespec_add_ms(struct timespec *ts, uint32_t ms)
{
	bool oflow;

	ts->tv_nsec += ms * NSEC_PER_MSEC;
	oflow = ts->tv_nsec >= NSEC_PER_SEC;
	ts->tv_sec += oflow;
	ts->tv_nsec -= oflow * NSEC_PER_SEC;
}

#endif
