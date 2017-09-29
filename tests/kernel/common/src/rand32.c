/* test random number generator APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This module tests the following random number routines:
 * u32_t sys_rand32_get(void);
 */

#include <ztest.h>
#include <logging/sys_log.h>

#define N_VALUES 10

/**
 *
 * @brief Regression test's entry point
 *
 *
 * @return N/A
 */

void rand32_test(void)
{
	u32_t rnd_values[N_VALUES];
	int i;

	/*
	 * Test subsequently calls sys_rand32_get(), checking
	 * that two values are not equal.
	 */
	SYS_LOG_DBG("Generating random numbers");
	/*
	 * Get several subsequent numbers as fast as possible.
	 * If random number generator is based on timer, check
	 * the situation when random number generator is called
	 * faster than timer clock ticks.
	 * In order to do this, make several subsequent calls
	 * and save results in an array to verify them on the
	 * next step
	 */
	for (i = 0; i < N_VALUES; i++) {
		rnd_values[i] = sys_rand32_get();
	}
	for (i = 1; i <  N_VALUES; i++) {
		zassert_false((rnd_values[i - 1] == rnd_values[i]),
			      "random number subsequent calls return same value");
	}

}
