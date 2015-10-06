/* test random number generator APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * DESCRIPTION
 * This module tests the following random number routines:
 * uint32_t sys_rand32_get(void);
 */

#include <tc_util.h>
#include <zephyr.h>

#define N_VALUES 10

/**
 *
 * @brief Regression test's entry point
 *
 *
 * @return N/A
 */

void RegressionTaskEntry(void)
{
	int  tc_result; /* test result code */
	uint32_t rnd_values[N_VALUES];
	int i;

	PRINT_DATA("Starting random number tests\n");
	PRINT_LINE;

	/*
	 * Test subsequently calls sys_rand32_get(), checking
	 * that two values are not equal.
	 */
	PRINT_DATA("Generating random numbers\n");
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
	for (tc_result = TC_PASS, i = 1; i <  N_VALUES; i++) {
		if (rnd_values[i - 1] == rnd_values[i]) {
			tc_result = TC_FAIL;
			break;
		}
	}

	if (tc_result == TC_FAIL) {
		TC_ERROR("random number subsequent calls\n"
			 "returned same value %d\n", rnd_values[i]);
	} else {
		PRINT_DATA("Generated %d values with expected randomness\n",
			   N_VALUES);
	}

	TC_END_RESULT(tc_result);
	TC_END_REPORT(tc_result);
}
