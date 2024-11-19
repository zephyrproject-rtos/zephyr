/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 * Copyright (c) 2016-2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Measure time
 *
 */
#include <zephyr/kernel.h>
#include <ksched.h>

#include "footprint.h"

#define STACK_SIZE	512

K_SEM_DEFINE(yield_sem, 0, 1);

void test_thread_entry(void *p, void *p1, void *p2)
{
	static int i;

	i++;
}

void thread_swap(void *p1, void *p2, void *p3)
{
	k_thread_abort(arch_current_thread());
}

void thread_suspend(void *p1, void *p2, void *p3)
{
	k_thread_suspend(arch_current_thread());
}

void thread_yield0(void *p1, void *p2, void *p3)
{
	uint32_t count = 0;

	k_sem_take(&yield_sem, K_MSEC(10));

	while (count != 1000U) {
		count++;
		k_yield();
	}
}

void thread_yield1(void *p1, void *p2, void *p3)
{
	k_sem_give(&yield_sem);
	while (1) {
		k_yield();
	}
}

void run_thread_system(void)
{
	int prio;
	k_tid_t yield0_tid;
	k_tid_t yield1_tid;

	k_tid_t my_tid = k_thread_create(&my_thread, my_stack_area,
					 STACK_SIZE,
					 thread_swap,
					 NULL, NULL, NULL,
					 5 /*priority*/, 0, K_FOREVER);

	k_thread_priority_set(my_tid, 5);
	prio = k_thread_priority_get(my_tid);
	if (prio != 5) {
		printk("thread priority is not set to 5!\n");
	}

	k_thread_start(my_tid);

	k_thread_abort(my_tid);

	k_tid_t sus_res_tid = k_thread_create(&my_thread, my_stack_area,
					      STACK_SIZE,
					      thread_suspend,
					      NULL, NULL, NULL,
					      -1 /*priority*/, 0, K_NO_WAIT);

	k_thread_resume(sus_res_tid);

	k_thread_join(sus_res_tid, K_FOREVER);

	k_sleep(K_MSEC(10));

	yield0_tid = k_thread_create(&my_thread, my_stack_area,
				     STACK_SIZE,
				     thread_yield0,
				     NULL, NULL, NULL,
				     0 /*priority*/, 0, K_NO_WAIT);

	yield1_tid = k_thread_create(&my_thread_0, my_stack_area_0,
				     STACK_SIZE,
				     thread_yield1,
				     NULL, NULL, NULL,
				     0 /*priority*/, 0, K_NO_WAIT);

	k_sleep(K_MSEC(1000));

	k_thread_abort(yield0_tid);
	k_thread_abort(yield1_tid);
}
