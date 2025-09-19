/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_LIB_CLIB_SRC_TEST_THRD_H_
#define TESTS_LIB_CLIB_SRC_TEST_THRD_H_

#include <stdint.h>
#include <time.h>

#include <zephyr/sys_clock.h>
#include <zephyr/sys/timeutil.h>

/* arbitrary magic numbers used for testing */
#define BIOS_FOOD     0xb105f00d
#define FORTY_TWO     42
#define SEVENTY_THREE 73
#define DONT_CARE     0x370ca2e5

static inline void timespec_add_ms(struct timespec *ts, uint32_t ms)
{
	struct timespec ms_as_ts = {
		.tv_sec = ms / MSEC_PER_SEC,
		.tv_nsec = (ms % MSEC_PER_SEC) * NSEC_PER_MSEC,
	};

	(void)timespec_add(ts, &ms_as_ts);
}

#endif
