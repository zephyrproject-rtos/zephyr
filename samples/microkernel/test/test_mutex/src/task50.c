/* task50.c - helper file for testing microkernel mutex APIs */

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
DESCRIPTION
This module defines a task that is used in recursive mutex locking tests.
It helps ensure that a private mutex can be referenced in a file other than
the one it was defined in.
*/

#include <tc_util.h>
#include <zephyr.h>

#define  ONE_SECOND                 (sys_clock_ticks_per_sec)
#define  HALF_SECOND                (sys_clock_ticks_per_sec / 2)
#define  THIRD_SECOND               (sys_clock_ticks_per_sec / 3)
#define  FOURTH_SECOND              (sys_clock_ticks_per_sec / 4)

static int tcRC = TC_PASS;         /* test case return code */

extern const kmutex_t private_mutex;

/**
 *
 * Task50 - task that participates in recursive locking tests
 *
 * @return  N/A
 */

void Task50(void)
{
	int  rv;

	/* Wait for private mutex to be released */

	rv = task_mutex_lock_wait(private_mutex);
	if (rv != RC_OK) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to obtain private mutex\n");
		return;
	}

	/* Wait a bit, then release the mutex */

	task_sleep(HALF_SECOND);
	task_mutex_unlock(private_mutex);

}   /* Task50 */
