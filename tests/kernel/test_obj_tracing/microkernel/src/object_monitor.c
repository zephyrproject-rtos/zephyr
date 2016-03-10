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

#ifdef CONFIG_NANOKERNEL
#define OBJ_LIST_NAME nano_sem
#define OBJ_LIST_TYPE struct nano_sem
/* We expect N_PHILOSPHERS fibers and:
 *	1 Background task
 *	1 The object monitor fiber
 */
#define DELTA_THREADS 2

static inline int test_task_tracing(void)
{
	return 1;
}

#else  /*CONFIG_MICROKERNEL*/
#define OBJ_LIST_NAME micro_mutex
#define OBJ_LIST_TYPE struct _k_mutex_struct
/* We expect N_PHILOSPHERS tasks and:
 *	1 Phil demo task
 *	1 The object monitor task
 *	1 Task scheduler fiber
 */
#define DELTA_THREADS 3

static inline int test_task_tracing(void)
{
	int obj_counter = 0;
	struct k_task *task_list = NULL;

	task_list   = (struct k_task *)SYS_TRACING_HEAD(struct k_task, micro_task);
	while (task_list != NULL) {
		TC_PRINT("TASK ID: 0x%x, PRIORITY: %d, GROUP %d\n",
			task_list->id, task_list->priority, task_list->group);
		task_list = (struct k_task *)SYS_TRACING_NEXT
				(struct k_task, micro_task, task_list);
		obj_counter++;
	}
	TC_PRINT("TASK QUANTITY: %d\n", obj_counter);

	/*k_server fiber does not have a k_task structure*/
	if (obj_counter == N_PHILOSOPHERS + DELTA_THREADS - 1) {
		TC_END_RESULT(TC_PASS);
		return 1;
	}

	TC_END_RESULT(TC_FAIL);
	return 0;
}

#endif  /*CONFIG_NANOKERNEL*/

static inline int test_thread_monitor(void)
{
	int obj_counter = 0;
	struct tcs *thread_list = NULL;

	thread_list   = (struct tcs *)SYS_THREAD_MONITOR_HEAD;
	while (thread_list != NULL) {
		if (thread_list->prio == -1) {
			TC_PRINT("TASK: %p FLAGS: 0x%x\n",
			thread_list, thread_list->flags);
		} else {
			TC_PRINT("FIBER: %p FLAGS: 0x%x\n",
			thread_list, thread_list->flags);
		}
		thread_list = (struct tcs *)SYS_THREAD_MONITOR_NEXT(thread_list);
		obj_counter++;
	}
	TC_PRINT("THREAD QUANTITY: %d\n", obj_counter);

	if (obj_counter == N_PHILOSOPHERS + DELTA_THREADS) {
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
		TC_PRINT("SEMAPHORE REF: %p\n", obj_list);
		obj_list = SYS_TRACING_NEXT(OBJ_LIST_TYPE, OBJ_LIST_NAME, obj_list);
		obj_counter++;
	}
	TC_PRINT("SEMAPHORE QUANTITY: %d\n", obj_counter);

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
