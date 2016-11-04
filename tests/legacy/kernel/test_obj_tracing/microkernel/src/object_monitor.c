/* object_monitor.c - object monitor */

/*
 * Copyright (c) 2016 Intel Corporation.
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

#include <zephyr.h>
#include <tc_util.h>
#include <util_test_common.h>
#include <misc/debug/object_tracing.h>
#include "phil.h"

/**
 *
 * @brief Thread that traverses, counts and reports
 * the kernel objects in the philosophers application.
 *
 */

#define TOTAL_TEST_NUMBER 3

/* 1 IPM console fiber if enabled */
#if defined(CONFIG_IPM_CONSOLE_RECEIVER) && defined(CONFIG_PRINTK)
#define IPM_THREAD 1
#else
#define IPM_THREAD 0
#endif /* CONFIG_IPM_CONSOLE_RECEIVER && CONFIG_PRINTK*/

#if defined(CONFIG_KERNEL_V2)

/* Must account for:
 *	N Philosopher threads
 *	1 Object monitor thread
 *	1 System idle thread
 *	1 System workqueue thread
 *	1 IPM console thread
 */

#define TOTAL_THREADS (N_PHILOSOPHERS + 3 + IPM_THREAD)

void *force_sys_work_q_in = (void *)&k_sys_work_q;

#define OBJ_LIST_NAME k_mutex
#define OBJ_LIST_TYPE struct k_mutex

/* unified kernel doesn't support task-specific tracing, so it always passes */
#define test_task_tracing() 1

#else

/* Must account for:
 *	N Philosopher tasks
 *	1 Object monitor task
 *	1 Microkernel idle task
 *	1 Microkernel server fiber
 *	1 IPM console fiber
 *
 * Also have philosopher demo task, but it terminates early on during the test
 * so we don't need to account for it.
 */

#define TOTAL_FIBERS (1 + IPM_THREAD)
#define TOTAL_TASKS (N_PHILOSOPHERS + 2)
#define TOTAL_THREADS (TOTAL_FIBERS + TOTAL_TASKS)

#define OBJ_LIST_NAME micro_mutex
#define OBJ_LIST_TYPE struct _k_mutex_struct

static inline int test_task_tracing(void)
{
	int obj_counter = 0;
	struct k_task *task_list =
		(struct k_task *)SYS_TRACING_HEAD(struct k_task, micro_task);

	while (task_list != NULL) {
		TC_PRINT("TASK ID: 0x%x, PRIORITY: %d, GROUP %d\n",
			task_list->id, task_list->priority, task_list->group);
		task_list = (struct k_task *)SYS_TRACING_NEXT
				(struct k_task, micro_task, task_list);
		obj_counter++;
	}
	TC_PRINT("TASK QUANTITY: %d\n", obj_counter);

	if (obj_counter == TOTAL_TASKS) {
		TC_END_RESULT(TC_PASS);
		return 1;
	}

	TC_END_RESULT(TC_FAIL);
	return 0;
}

#endif /* CONFIG_KERNEL_V2 */

static inline int test_thread_monitor(void)
{
	int obj_counter = 0;
	struct tcs *thread_list = NULL;

	/* wait a bit to allow any initialization-only threads to terminate */
	task_sleep(100);

	thread_list   = (struct tcs *)SYS_THREAD_MONITOR_HEAD;
	while (thread_list != NULL) {
		if (thread_list->prio == -1) {
			TC_PRINT("TASK: %p FLAGS: 0x%x\n",
			thread_list, thread_list->flags);
		} else {
			TC_PRINT("FIBER: %p FLAGS: 0x%x\n",
			thread_list, thread_list->flags);
		}
		thread_list =
			(struct tcs *)SYS_THREAD_MONITOR_NEXT(thread_list);
		obj_counter++;
	}
	TC_PRINT("THREAD QUANTITY: %d\n", obj_counter);

	if (obj_counter == TOTAL_THREADS) {
		TC_END_RESULT(TC_PASS);
		return 1;
	}

	TC_END_RESULT(TC_FAIL);
	return 0;
}

void object_monitor(void)
{
	int obj_counter;
	int test_counter = 0;
	void *obj_list   = NULL;

	TC_START("OBJECT TRACING TEST");

	obj_counter = 0;
	obj_list   = SYS_TRACING_HEAD(OBJ_LIST_TYPE, OBJ_LIST_NAME);
	while (obj_list != NULL) {
		TC_PRINT("MUTEX REF: %p\n", obj_list);
		obj_list = SYS_TRACING_NEXT(OBJ_LIST_TYPE, OBJ_LIST_NAME,
						obj_list);
		obj_counter++;
	}
	TC_PRINT("MUTEX QUANTITY: %d\n", obj_counter);

	if (obj_counter == N_PHILOSOPHERS) {
		TC_END_RESULT(TC_PASS);
		test_counter++;
	} else {
		TC_END_RESULT(TC_FAIL);
	}

	test_counter += test_thread_monitor();

	test_counter += test_task_tracing();

	if (test_counter == TOTAL_TEST_NUMBER) {
		TC_END_REPORT(TC_PASS);
	} else {
		TC_END_REPORT(TC_FAIL);
	}
}
