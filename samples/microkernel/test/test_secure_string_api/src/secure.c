/* secure.c - test microkernel secure API under VxMicro */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
This modules tests the following secure routines:
  strcpy_s, strlen_s, k_memcpy_s
*/

/* includes */

#include <vxmicro.h>
#include <tc_util.h>
#include <string_s.h>

/* timeout to wait for the task to start */
#define WAIT_TOUT (sys_clock_ticks_per_sec)

/* default buffer size */
#define BUFSIZE 10

/*******************************************************************************
 *
 * MainTask - the main test task
 *
 * Starts testing tasks and checks their statuses
 *
 * RETURNS: N/A
 */

void MainTask (void)
	{
	int result;
    /* wait for the first task to start */
	result = task_sem_take_wait_timeout (SEMA1, WAIT_TOUT);
	if (result != RC_OK) {
	TC_ERROR ("Test task 1 did not start properly\n");
	goto fail;
	}
    /* now we check the first task to perform the test and die */
	result = task_sem_take_wait_timeout (SEMA1, WAIT_TOUT);
	if (result == RC_TIME) {
	TC_PRINT ("As expected, test task 1 did not continue operating \n");
	TC_PRINT ("after calling memcpy_s with incorrect parameters\n");
	}
	else {
	TC_ERROR ("Test task 1 unexpectedly continued\n"
		  "after calling memcpy_s with incorrect parameters\n");
	goto fail;
	}

    /* wait for the second task to start */
	result = task_sem_take_wait_timeout (SEMA2, WAIT_TOUT);
	if (result != RC_OK) {
	TC_ERROR ("Test task 2 did not start properly\n");
	goto fail;
	}
    /* now we check the second task to perform the test and die */
	result = task_sem_take_wait_timeout (SEMA2, WAIT_TOUT);
	if (result == RC_TIME) {
	TC_PRINT ("As expected, test task 2 did not continue operating \n");
	TC_PRINT ("after calling strcpy_s with incorrect parameters\n");
	}
	else {
	TC_ERROR ("Test task 2 unexpectedly continued\n"
		  "after calling memcpy_s with incorrect parameters\n");
	goto fail;
	}
	TC_END_RESULT (TC_PASS);
	TC_END_REPORT (TC_PASS);
	return;

fail:
	TC_END_RESULT (TC_FAIL);
	TC_END_REPORT (TC_FAIL);
	}

/*******************************************************************************
 *
 * MemcpyTask - k_memcpy_s test task
 *
 * Task tests k_memcpy_s function
 *
 * RETURNS: N/A
 */
void MemcpyTask (void)
	{
	uint8_t buf1[BUFSIZE];
	uint8_t buf2[BUFSIZE + 1];

    /* do correct memory copy */
	k_memcpy_s (buf2, sizeof (buf2), buf1, sizeof (buf1));
	task_sem_give (SEMA1);
	task_yield ();
    /* do incorrect memory copy */
	k_memcpy_s (buf1, sizeof (buf1), buf2, sizeof (buf2));
	task_sem_give (SEMA1);
	}

/*******************************************************************************
 *
 * StrcpyTask - strcpy_s test task
 *
 * Task tests strcpy_s function
 *
 * RETURNS: N/A
 */
void StrcpyTask (void)
	{
	char buf1[BUFSIZE];
	char buf2[BUFSIZE] = { '1', '2', '3', '4', '5',
			   '6', '7', '8', '9', 0 };

    /* do correct string copy */
	strcpy_s (buf1, sizeof (buf1), buf2);
	task_sem_give (SEMA2);
	task_yield ();

    /*
     * Make the string not \0 terminated as a result
     * strcpy_s has to make an error
     */
	buf2[BUFSIZE - 1] = '0';
    /* do incorrect string copy */
	strcpy_s (buf1, sizeof (buf1), buf2);
	task_sem_give (SEMA2);
	}
