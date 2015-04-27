/* test_xip.c - test XIP */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
DESCRIPTION
This module tests that XIP performs as expected. If the first
task is even activated that is a good indication that XIP is
working. However, the test does do some some testing on
global variables for completeness sake.
*/

/* includes */

#include <tc_util.h>
#include <nanokernel.h>
#include "test.h"


#if defined(CONFIG_NANOKERNEL)
/*******************************************************************************
*
* main - main task entry point
*
* Entry point for nanokernel only builds.
*
* RETURNS: N/A
*/

void main (void)
#else
/*******************************************************************************
*
* RegressionTaskEntry - regression test's entry point
*
* RETURNS: N/A
*/

void RegressionTaskEntry (void)
#endif
	{
	int  tcRC = TC_PASS;
	int  i;

	PRINT_DATA ("Starting XIP tests\n");
	PRINT_LINE;

    /* Test globals are correct */

	TC_PRINT ("Test globals\n");

    /* Array should be filled with monotomically incrementing values */
	for (i = 0; i < XIP_TEST_ARRAY_SZ; i++)
	{
	if (xip_array[i] != (i+1))
	    {
	    TC_PRINT ("xip_array[%d] != %d\n", i, i+1);
	    tcRC = TC_FAIL;
		    goto exitRtn;
	    }
	}

exitRtn:
	TC_END_RESULT (tcRC);
	TC_END_REPORT (tcRC);
	}
