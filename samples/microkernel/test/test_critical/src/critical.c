/* critical.c - test the task_offload_to_fiber() API */

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
This module tests the task_offload_to_fiber() API.
*/

#include <zephyr.h>
#include <tc_util.h>
#include <sections.h>

#define NUM_TICKS    500
#define TEST_TIMEOUT 2000

static uint32_t  criticalVar = 0;
static uint32_t altTaskIterations = 0;

/*******************************************************************************
*
* criticalRtn - routine to be called from K_swapper()
*
* This routine increments the global variable <criticalVar>.
*
* RETURNS: 0
*/

int criticalRtn(void)
{
	volatile  uint32_t x;

	x = criticalVar;
	criticalVar = x + 1;

	return 0;
}

/*******************************************************************************
*
* criticalLoop - common code for invoking task_offload_to_fiber()
*
* \param count    number of critical section calls made thus far
*
* RETURNS: number of critical section calls made by task
*/

uint32_t criticalLoop(uint32_t count)
{
	int32_t  ticks;

	ticks = task_tick_get_32();
	while (task_tick_get_32() < ticks + NUM_TICKS) {
		task_offload_to_fiber(criticalRtn, &criticalVar);
		count++;
	}

	return count;
}

/*******************************************************************************
*
* AlternateTask - alternate task
*
* This routine calls task_offload_to_fiber() many times.
*
* RETURNS: N/A
*/

void AlternateTask(void)
{
	task_sem_take_wait(ALT_SEM);     /* Wait to be activated */

	altTaskIterations = criticalLoop(altTaskIterations);

	task_sem_give(REGRESS_SEM);

	task_sem_take_wait(ALT_SEM);    /* Wait to be re-activated */

	altTaskIterations = criticalLoop(altTaskIterations);

	task_sem_give(REGRESS_SEM);
}

/*******************************************************************************
*
* RegressionTask - regression task
*
* This routine calls task_offload_to_fiber() many times.  It also checks to
* ensure that the number of times it is called matches the global variable
* <criticalVar>.
*
* RETURNS: N/A
*/

void RegressionTask(void)
{
	uint32_t nCalls = 0;
	int      status;

	TC_START("Test Microkernel Critical Section API\n");

	task_sem_give(ALT_SEM);      /* Activate AlternateTask() */

	nCalls = criticalLoop(nCalls);

	/* Wait for AlternateTask() to complete */
	status = task_sem_take_wait_timeout(REGRESS_SEM, TEST_TIMEOUT);
	if (status != RC_OK) {
		TC_ERROR("Timed out waiting for REGRESS_SEM\n");
		goto errorReturn;
	}

	if (criticalVar != nCalls + altTaskIterations) {
		TC_ERROR("Unexpected value for <criticalVar>.  Expected %d, got %d\n",
				 nCalls + altTaskIterations, criticalVar);
		goto errorReturn;
	}
	TC_PRINT("Obtained expected <criticalVar> value of %u\n", criticalVar);

	TC_PRINT("Enabling time slicing ...\n");

	scheduler_time_slice_set(1, 10);

	task_sem_give(ALT_SEM);      /* Re-activate AlternateTask() */

	nCalls = criticalLoop(nCalls);

	/* Wait for AlternateTask() to finish */
	status = task_sem_take_wait_timeout(REGRESS_SEM, TEST_TIMEOUT);
	if (status != RC_OK) {
		TC_ERROR("Timed out waiting for REGRESS_SEM\n");
		goto errorReturn;
	}

	if (criticalVar != nCalls + altTaskIterations) {
		TC_ERROR("Unexpected value for <criticalVar>.  Expected %d, got %d\n",
				 nCalls + altTaskIterations, criticalVar);
		goto errorReturn;
	}
	TC_PRINT("Obtained expected <criticalVar> value of %u\n", criticalVar);

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
	return;

errorReturn:
	TC_END_RESULT(TC_FAIL);
	TC_END_REPORT(TC_FAIL);
}
