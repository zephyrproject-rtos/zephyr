/* libraries.c - test access to VxMicro standard libraries */

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
This module verifies that the various standard libraries supported by VxMicro
can be used.

IMPORTANT: The module only ensures that each supported library is present,
and that a bare minimum of its functionality is operating correctly. It does
NOT guarantee that ALL standards-defined functionality is present, nor does
it guarantee that ALL functionality provided is working correctly.
*/

/* includes */

#include <microkernel.h>
#include <tc_util.h>

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * variables used during limits library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile long longMax = LONG_MAX;
volatile long longOne = 1L;

/*******************************************************************************
*
* limitsTest - test implementation-defined constants library
*
* RETURNS: TC_PASS or TC_FAIL
*/

int limitsTest(void)
	{
	TC_PRINT("Testing limits.h library ...\n");

	if (longMax + longOne != LONG_MIN)
	return TC_FAIL;

	return TC_PASS;
	}

/*******************************************************************************
*
* stdboolTest - test boolean types and values library
*
* RETURNS: TC_PASS or TC_FAIL
*/

int stdboolTest(void)
	{
	TC_PRINT("Testing stdbool.h library ...\n");

	if ((true != 1) || (false != 0))
	return TC_FAIL;

	return TC_PASS;
	}

/*
 * variables used during stddef library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile long longVariable;
volatile size_t sizeOfLongVariable = sizeof(longVariable);

/*******************************************************************************
*
* stddefTest - test standard type definitions library
*
* RETURNS: TC_PASS or TC_FAIL
*/

int stddefTest(void)
	{
	TC_PRINT("Testing stddef.h library ...\n");

	if (sizeOfLongVariable != 4)
	return TC_FAIL;

	return TC_PASS;
	}

/*
 * variables used during stdint library testing; must be marked as "volatile"
 * to prevent compiler from computing results at compile time
 */

volatile uint8_t unsignedByte = 0xff;
volatile uint32_t unsignedInt = 0xffffff00;

/*******************************************************************************
*
* stdintTest - test integer types library
*
* RETURNS: TC_PASS or TC_FAIL
*/

int stdintTest(void)
	{
	TC_PRINT("Testing stdint.h library ...\n");

	if (unsignedInt + unsignedByte + 1u != 0)
	return TC_FAIL;

	return TC_PASS;
	}

/*
 * variables used during string library testing
 */

#define BUFSIZE 10

char buffer[BUFSIZE];

/*******************************************************************************
*
* stringTest - test string operations library
*
* RETURNS: TC_PASS or TC_FAIL
*/

int stringTest(void)
	{
	TC_PRINT("Testing string.h library ...\n");

	memset(buffer, 'a', BUFSIZE);
	if (buffer[0] != 'a' || buffer[BUFSIZE-1] != 'a')
	return TC_FAIL;

	return TC_PASS;
	}

/*******************************************************************************
*
* RegressionTask - main task in the test suite
*
* This is the entry point to the main task used by the standard libraries test
* suite. It tests each library in turn until a failure is detected or all
* libraries have been tested successfully.
*
* RETURNS: TC_PASS or TC_FAIL
*/

int RegressionTask(void)
	{
	TC_PRINT("Validating access to supported libraries\n");

	if (limitsTest () || stdboolTest () || stddefTest () ||
	stdintTest () || stringTest ()) {
	TC_PRINT ("Library validation failed\n");
	return TC_FAIL;
	}

	TC_PRINT("Validation complete\n");
	return TC_PASS;
	}
