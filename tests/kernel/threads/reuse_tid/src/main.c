/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#define TEST_THREAD_STACKSIZE 2048

volatile bool expect_fault;

#define TEST_THREAD_CREATOR_PRIORITY (CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#define TEST_THREAD_REUSED_PRIORITY  (CONFIG_NUM_PREEMPT_PRIORITIES - 2)

struct k_thread test_thread_creator;
struct k_thread test_thread_reused;

atomic_t thread_reuse_count;

static K_KERNEL_STACK_DEFINE(test_thread_creator_stack, TEST_THREAD_STACKSIZE);
static K_KERNEL_STACK_DEFINE(test_thread_reused_stack, TEST_THREAD_STACKSIZE);

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);

	if (expect_fault && reason == K_ERR_KERNEL_PANIC) {
		expect_fault = false;
		ztest_test_pass();
	} else {
		printk("Unexpected fault during test\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

void reused_thread_waitforever(void *p1, void *p2, void *p3)
{
	printf("In wait forver reused thread\n");
	atomic_val_t prev_val = atomic_inc(&thread_reuse_count);
	if (prev_val == 0) {
		k_sleep(K_FOREVER);
	} else {
		/**
		 * using ztest_test_fail() here will not work
		 * since the thread states are corrupted
		 *
		 * so, raisng a fault to indicate failure
		 */
		z_except_reason(K_ERR_ARCH_START + 0x1000);
	}
}

void reused_thread_return(void *p1, void *p2, void *p3)
{
	printf("In immediate return reused thread\n");
}

static void creator_thread(void *p1, void *p2, void *p3)
{
	k_tid_t tid1, tid2;

	/* Test reusing TID after thread join */
	tid1 = k_thread_create(&test_thread_reused, test_thread_reused_stack,
			       K_KERNEL_STACK_SIZEOF(test_thread_reused_stack),
			       reused_thread_return, NULL, NULL, NULL,
			       TEST_THREAD_REUSED_PRIORITY, 0, K_NO_WAIT);
	k_thread_join(&test_thread_reused, K_FOREVER);

	/* Try to reuse the same thread structure */
	tid2 = k_thread_create(&test_thread_reused, test_thread_reused_stack,
			       K_KERNEL_STACK_SIZEOF(test_thread_reused_stack),
			       reused_thread_return, NULL, NULL, NULL,
			       TEST_THREAD_REUSED_PRIORITY, 0, K_NO_WAIT);
	k_thread_join(&test_thread_reused, K_FOREVER);

	/* Test reusing TID before thread join */
	tid1 = k_thread_create(&test_thread_reused, test_thread_reused_stack,
			       K_KERNEL_STACK_SIZEOF(test_thread_reused_stack),
			       reused_thread_waitforever, NULL, NULL, NULL,
			       TEST_THREAD_REUSED_PRIORITY, 0, K_NO_WAIT);

	expect_fault = true;

	/* Try to reuse the same thread structure */
	tid2 = k_thread_create(&test_thread_reused, test_thread_reused_stack,
			       K_KERNEL_STACK_SIZEOF(test_thread_reused_stack),
			       reused_thread_waitforever, NULL, NULL, NULL,
			       TEST_THREAD_REUSED_PRIORITY, 0, K_NO_WAIT);

	if (tid2 != 0) {
		ztest_test_fail();
	}
}

ZTEST(reuse_tid, test_tid_reuse)
{
	k_tid_t tid;

	tid = k_thread_create(&test_thread_creator, test_thread_creator_stack,
			      K_KERNEL_STACK_SIZEOF(test_thread_creator_stack), creator_thread,
			      NULL, NULL, NULL, TEST_THREAD_CREATOR_PRIORITY, 0, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

	ztest_test_fail();
}

ZTEST_SUITE(reuse_tid, NULL, NULL, NULL, NULL, NULL);
