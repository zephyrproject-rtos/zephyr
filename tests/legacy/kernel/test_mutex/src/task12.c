/* Task12.c - helper file for testing microkernel mutex APIs */

/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
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
 * @file
 * @brief mutex test helper
 *
 * This module defines a task that is used in recursive mutex locking tests.
 * It helps ensure that a private mutex can be referenced in a file other than
 * the one it was defined in.
 */

#include <tc_util.h>
#include <zephyr.h>

#define  ONE_SECOND                 (sys_clock_ticks_per_sec)
#define  HALF_SECOND                (sys_clock_ticks_per_sec / 2)
#define  THIRD_SECOND               (sys_clock_ticks_per_sec / 3)
#define  FOURTH_SECOND              (sys_clock_ticks_per_sec / 4)

static int tcRC = TC_PASS;         /* test case return code */

extern kmutex_t const private_mutex;

/**
 *
 * Task12 - task that participates in recursive locking tests
 *
 * @return  N/A
 */

void Task12(void)
{
	int  rv;

	/* Wait for private mutex to be released */

	rv = task_mutex_lock(private_mutex, TICKS_UNLIMITED);
	if (rv != RC_OK) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to obtain private mutex\n");
		return;
	}

	/* Wait a bit, then release the mutex */

	task_sleep(HALF_SECOND);
	task_mutex_unlock(private_mutex);

}
