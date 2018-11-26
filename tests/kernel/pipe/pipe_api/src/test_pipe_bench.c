/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel_structs.h>
#include <ksched.h>
#include <wait_q.h>

#if CONFIG_X86
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME() (_tsc_read())
#elif CONFIG_ARM
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (k_cycle_get_32())
#endif

#define STACK_SIZE      1024
#define PIPE_LEN 16
K_PIPE_DEFINE(kpipe4, PIPE_LEN, 4);
K_THREAD_STACK_DEFINE(tstack4, STACK_SIZE);
K_THREAD_STACK_DEFINE(tstack5, STACK_SIZE);
K_THREAD_STACK_DEFINE(tstack6, STACK_SIZE);
K_THREAD_STACK_DEFINE(tstack7, STACK_SIZE);
__kernel struct k_thread tdata4;
__kernel struct k_thread tdata5;
__kernel struct k_thread tdata6;
__kernel struct k_thread tdata7;


static void thread_handler(void *p1, void *p2, void *p3)
{
	while (1) {
		k_yield();
	}
}

void test_bench(void)
{
	struct k_thread *thread;

	k_tid_t tid1 = k_thread_create(&tdata4, tstack4, STACK_SIZE,
					thread_handler, NULL, NULL, NULL,
					K_PRIO_PREEMPT(1), 0, 0);

	k_tid_t tid2 = k_thread_create(&tdata5, tstack5, STACK_SIZE,
					thread_handler, NULL, NULL, NULL,
					K_PRIO_PREEMPT(2), 0, 0);

	k_tid_t tid3 = k_thread_create(&tdata6, tstack6, STACK_SIZE,
					thread_handler, NULL, NULL, NULL,
					K_PRIO_PREEMPT(3), 0, 0);

	k_tid_t tid4 = k_thread_create(&tdata7, tstack7, STACK_SIZE,
					thread_handler, &kpipe4, NULL, NULL,
					K_PRIO_PREEMPT(4), 0, 0);

	k_sleep(10);

	TIMING_INFO_PRE_READ();
	u32_t pre_pend_time1 = TIMING_INFO_OS_GET_TIME();

	_pend_thread((struct k_thread *) tid1,
			&kpipe4.wait_q.writers, K_NO_WAIT);

	TIMING_INFO_PRE_READ();
	u32_t post_pend_time1 = TIMING_INFO_OS_GET_TIME();
	u32_t time_for_pend1 = (post_pend_time1 - pre_pend_time1);

	TIMING_INFO_PRE_READ();
	u32_t pre_pend_time2 = TIMING_INFO_OS_GET_TIME();

	_pend_thread((struct k_thread *) tid2,
			&kpipe4.wait_q.writers, K_NO_WAIT);

	TIMING_INFO_PRE_READ();
	u32_t post_pend_time2 = TIMING_INFO_OS_GET_TIME();
	u32_t time_for_pend2 = (post_pend_time2 - pre_pend_time2);

	TIMING_INFO_PRE_READ();
	u32_t pre_pend_time3 = TIMING_INFO_OS_GET_TIME();

	_pend_thread((struct k_thread *) tid3,
			&kpipe4.wait_q.writers, K_NO_WAIT);

	TIMING_INFO_PRE_READ();
	u32_t post_pend_time3 = TIMING_INFO_OS_GET_TIME();
	u32_t time_for_pend3 = (post_pend_time3 - pre_pend_time3);

	TIMING_INFO_PRE_READ();
	u32_t pre_pend_time4 = TIMING_INFO_OS_GET_TIME();

	_pend_thread((struct k_thread *) tid4,
			&kpipe4.wait_q.writers, K_NO_WAIT);

	TIMING_INFO_PRE_READ();
	u32_t post_pend_time4 = TIMING_INFO_OS_GET_TIME();
	u32_t time_for_pend4 = (post_pend_time4 - pre_pend_time4);

	printk("time spent during pending for thread1  %d\n", time_for_pend1);
	printk("time spent during pending for thread2  %d\n", time_for_pend2);
	printk("time spent during pending for thread3  %d\n", time_for_pend3);
	printk("time spent during pending for thread4  %d\n", time_for_pend4);

	while ((thread = _waitq_head(&kpipe4.wait_q.writers)) != NULL) {
		TIMING_INFO_PRE_READ();
		u32_t pre_unpend_time = TIMING_INFO_OS_GET_TIME();

		_unpend_thread(thread);

		TIMING_INFO_PRE_READ();
		u32_t post_unpend_time = TIMING_INFO_OS_GET_TIME();

		u32_t time_for_unpend = post_unpend_time - pre_unpend_time;

		printk("time spent during unpend of thread %d\n",
							time_for_unpend);
	}
}
