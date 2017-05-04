/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_SCHED_H__
#define __TEST_SCHED_H__

#include <zephyr.h>
#include <ztest.h>

#if defined(CONFIG_RISCV32)
#define STACK_SIZE 512
#else
#define STACK_SIZE (256 + CONFIG_TEST_EXTRA_STACKSIZE)
#endif

struct thread_data {
	k_tid_t tid;
	int priority;
	int executed;
};

void test_priority_cooperative(void);
void test_priority_preemptible(void);
void test_yield_cooperative(void);
void test_sleep_cooperative(void);
void test_sleep_wakeup_preemptible(void);
void test_time_slicing_preemptible(void);
void test_time_slicing_disable_preemptible(void);
void test_lock_preemptible(void);
void test_unlock_preemptible(void);
void test_sched_is_preempt_thread(void);
void test_slice_reset(void);

#endif /* __TEST_SCHED_H__ */
