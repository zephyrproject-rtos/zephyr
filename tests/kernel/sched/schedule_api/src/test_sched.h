/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_SCHED_H__
#define __TEST_SCHED_H__

#include <zephyr.h>
#include <ztest.h>

#define MAX_NUM_THREAD 10
#define STACK_SIZE (640 + CONFIG_TEST_EXTRA_STACKSIZE)

K_THREAD_STACK_EXTERN(tstack);
extern k_thread_stack_t
	tstacks[MAX_NUM_THREAD][K_THREAD_STACK_LEN(STACK_SIZE)];

struct thread_data {
	k_tid_t tid;
	int priority;
	int executed;
};

void spin_for_ms(int ticks);

void test_priority_cooperative(void);
void test_priority_preemptible(void);
void test_yield_cooperative(void);
void test_sleep_cooperative(void);
void test_sleep_wakeup_preemptible(void);
void test_pending_thread_wakeup(void);
void test_time_slicing_preemptible(void);
void test_time_slicing_disable_preemptible(void);
void test_lock_preemptible(void);
void test_unlock_preemptible(void);
void test_sched_is_preempt_thread(void);
void test_slice_reset(void);
void test_slice_scheduling(void);
void test_priority_scheduling(void);
void test_wakeup_expired_timer_thread(void);

#endif /* __TEST_SCHED_H__ */
