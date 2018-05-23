/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>
#include <debug/object_tracing.h>
#include "phil.h"

/**
 * @brief object monitor
 *
 * Thread that traverses, counts and reports the kernel objects in the
 * philosophers application.
 *
 */

#define TOTAL_TEST_NUMBER 2
#define ZTEST_OBJECT_NUM 1
#define ZTEST_THREADS_CREATED 1

/* 1 IPM console thread if enabled */
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

void *force_sys_work_q_in = (void *)&k_sys_work_q;

#define TOTAL_THREADS (N_PHILOSOPHERS + 3 + IPM_THREAD + ZTEST_THREADS_CREATED)
#define TOTAL_OBJECTS (N_PHILOSOPHERS + ZTEST_OBJECT_NUM)

#define OBJ_LIST_NAME k_sem
#define OBJ_LIST_TYPE struct k_sem
extern struct k_sem f3;

static inline int test_thread_monitor(void)
{
	int obj_counter = 0;
	struct k_thread *thread_list = NULL;

	/* wait a bit to allow any initialization-only threads to terminate */

	thread_list   = (struct k_thread *)SYS_THREAD_MONITOR_HEAD;
	while (thread_list != NULL) {
		if (thread_list->base.prio == -1) {
			TC_PRINT("PREMPT: %p OPTIONS: 0x%02x, STATE: 0x%02x\n",
				 thread_list,
				 thread_list->base.user_options,
				 thread_list->base.thread_state);
		} else {
			TC_PRINT("COOP: %p OPTIONS: 0x%02x, STATE: 0x%02x\n",
				 thread_list,
				 thread_list->base.user_options,
				 thread_list->base.thread_state);
		}
		thread_list =
			(struct k_thread *)SYS_THREAD_MONITOR_NEXT(thread_list);
		obj_counter++;
	}
	TC_PRINT("THREAD QUANTITY: %d\n", obj_counter);
	return obj_counter;
}

void object_monitor(void)
{
	int obj_counter;
	int thread_counter = 0;

	void *obj_list   = NULL;

	k_sem_take(&f3, 0);
	/* ztest use one semaphore so use one count less than expected to pass
	 * test
	 */
	obj_counter = -1;
	obj_list   = SYS_TRACING_HEAD(OBJ_LIST_TYPE, OBJ_LIST_NAME);
	while (obj_list != NULL) {
		TC_PRINT("SEMAPHORE REF: %p\n", obj_list);
		obj_list = SYS_TRACING_NEXT(OBJ_LIST_TYPE, OBJ_LIST_NAME,
					    obj_list);
		obj_counter++;
	}
	TC_PRINT("SEMAPHORE QUANTITY: %d\n", obj_counter);

	thread_counter += test_thread_monitor();

	zassert_true(((thread_counter == TOTAL_THREADS) &&
		   (obj_counter == TOTAL_OBJECTS)), "test failed");
}
