/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_SCHED_H__
#define __TEST_SCHED_H__

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define MAX_NUM_THREAD 10
#define STACK_SIZE (640 + CONFIG_TEST_EXTRA_STACK_SIZE)

K_THREAD_STACK_DECLARE(tstack, STACK_SIZE);
K_THREAD_STACK_ARRAY_DECLARE(tstacks, MAX_NUM_THREAD, STACK_SIZE);
extern struct k_thread user_thread;
extern struct k_sem user_sem;

struct thread_data {
	k_tid_t tid;
	int priority;
	int executed;
};

void spin_for_ms(int ticks);

void test_priority_cooperative(void);
void test_priority_preemptible(void);
void test_priority_preemptible_wait_prio(void);
void test_bad_priorities(void);
void test_yield_cooperative(void);
void test_sleep_cooperative(void);
void test_busy_wait_cooperative(void);
void test_sleep_wakeup_preemptible(void);
void test_pending_thread_wakeup(void);
void test_time_slicing_preemptible(void);
void test_time_slicing_disable_preemptible(void);
void test_lock_preemptible(void);
void test_unlock_preemptible(void);
void test_unlock_nested_sched_lock(void);
void test_sched_is_preempt_thread(void);
void test_slice_reset(void);
void test_slice_scheduling(void);
void test_priority_scheduling(void);
void test_wakeup_expired_timer_thread(void);
void test_user_k_wakeup(void);
void test_user_k_is_preempt(void);
void test_k_thread_suspend_init_null(void);
void test_k_thread_resume_init_null(void);
void test_k_thread_priority_get_init_null(void);
void test_k_thread_priority_set_init_null(void);
void test_k_thread_priority_set_overmax(void);
void test_k_thread_priority_set_upgrade(void);
void test_k_wakeup_init_null(void);
void test_slice_perthread(void);

#endif /* __TEST_SCHED_H__ */
