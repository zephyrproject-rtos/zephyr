/* object_monitor.c - object monitor */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <util_test_common.h>
#include <debug/object_tracing.h>
#include "phil.h"

/**
 *
 * @brief Thread that traverses, counts and reports
 * the kernel objects in the philosophers application.
 *
 */

#define TOTAL_TEST_NUMBER 2

/* 1 IPM console fiber if enabled */
#if defined(CONFIG_IPM_CONSOLE_RECEIVER) && defined(CONFIG_PRINTK)
#define IPM_THREAD 1
#else
#define IPM_THREAD 0
#endif /* CONFIG_IPM_CONSOLE_RECEIVER && CONFIG_PRINTK*/

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

static inline int test_thread_monitor(void)
{
	int obj_counter = 0;
	struct k_thread *thread_list = NULL;

	/* wait a bit to allow any initialization-only threads to terminate */
	task_sleep(100);

	thread_list   = (struct k_thread *)SYS_THREAD_MONITOR_HEAD;
	while (thread_list != NULL) {
		if (thread_list->base.prio == -1) {
			TC_PRINT("TASK: %p OPTIONS: 0x%02x, STATE: 0x%02x\n",
			thread_list,
			thread_list->base.user_options,
			thread_list->base.thread_state);
		} else {
			TC_PRINT("FIBER: %p OPTIONS: 0x%02x, STATE: 0x%02x\n",
			thread_list,
			thread_list->base.user_options,
			thread_list->base.thread_state);
		}
		thread_list =
			(struct k_thread *)SYS_THREAD_MONITOR_NEXT(thread_list);
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

	if (test_counter == TOTAL_TEST_NUMBER) {
		TC_END_REPORT(TC_PASS);
	} else {
		TC_END_REPORT(TC_FAIL);
	}
}
