/* test random number generator APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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
 * This module tests the following random number routines:
 * uint32_t sys_rand32_get(void);
 */

#include <tc_util.h>
#include <zephyr.h>

#define N_VALUES 10

/**
 *
 * RegressionTaskEntry - regression test's entry point
 *
 *
 * RETURNS: N/A
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
