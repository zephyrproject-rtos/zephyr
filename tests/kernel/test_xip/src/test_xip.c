/* test_xip.c - test XIP */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
DESCRIPTION
This module tests that XIP performs as expected. If the first
task is even activated that is a good indication that XIP is
working. However, the test does do some some testing on
global variables for completeness sake.
 */

#include <zephyr.h>
#include <tc_util.h>
#include "test.h"


/**
 *
 * @brief Regression test's entry point
 *
 * @return N/A
 */

void main(void)
{
	int  tcRC = TC_PASS;
	int  i;

	PRINT_DATA("Starting XIP tests\n");
	PRINT_LINE;

	/* Test globals are correct */

	TC_PRINT("Test globals\n");

	/* Array should be filled with monotomically incrementing values */
	for (i = 0; i < XIP_TEST_ARRAY_SZ; i++) {
		if (xip_array[i] != (i+1)) {
			TC_PRINT("xip_array[%d] != %d\n", i, i+1);
			tcRC = TC_FAIL;
			goto exitRtn;
		}
	}

exitRtn:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}
