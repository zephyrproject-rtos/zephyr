/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

volatile bool expect_fault;

#define TEST_THREADS_STACKSIZE 512

static K_KERNEL_STACK_DEFINE(kernel_thread_stack, TEST_THREADS_STACKSIZE);
static K_THREAD_STACK_DEFINE(user_thread_stack, TEST_THREADS_STACKSIZE);

char user_thread_invalid_stack[TEST_THREADS_STACKSIZE];

struct k_thread kernel_thread;
struct k_thread user_thread;

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

static void thread_function(void *p1, void *p2, void *p3)
{
	/* Empty thread function */
}

ZTEST(thread_stack_validate, test_thread_stack_validate)
{
	expect_fault = false;

	k_thread_create(&kernel_thread, kernel_thread_stack, TEST_THREADS_STACKSIZE,
			thread_function, NULL, NULL, NULL,
			-1, 0, K_NO_WAIT);

	k_thread_create(&user_thread, user_thread_stack, TEST_THREADS_STACKSIZE,
			thread_function, NULL, NULL, NULL,
			-1, K_USER, K_NO_WAIT);

	expect_fault = true;

	k_thread_create(&user_thread, (k_thread_stack_t *)user_thread_invalid_stack,
			TEST_THREADS_STACKSIZE, thread_function, NULL, NULL, NULL,
			-1, 0, K_NO_WAIT);

	ztest_test_fail();
}

ZTEST_SUITE(thread_stack_validate, NULL, NULL, NULL, NULL, NULL);
