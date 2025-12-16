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

#ifdef CONFIG_DYNAMIC_THREAD
static void validate_dynamic_stack(void)
{
#define NUM_STACKS_TO_VALIDATE (CONFIG_DYNAMIC_THREAD_POOL_SIZE + 2)

	const size_t stack_size = 1024;
	k_thread_stack_t *ptr[NUM_STACKS_TO_VALIDATE];

	/**
	 *  stack allocated for kernel is only valid for kernel threads and
	 * invalid for user threads
	 */
	for (size_t i = 0; i < NUM_STACKS_TO_VALIDATE; i++) {
		ptr[i] = k_thread_stack_alloc(stack_size, 0);
		if (ptr[i] != NULL) {
			/* Check if the stack is valid for kernel threads */
			zassert_true(k_thread_stack_is_valid(ptr[i], stack_size, 0),
				     "Dynamic stack is not valid");
#if CONFIG_DYNAMIC_THREAD_POOL_SIZE == 0
			/** k_thread_stack_alloc is common for both dynamic
			 * alloc dynamic pool of stacks. Stacks allocated in
			 * dynamic pool are valid for both kernel and user
			 * threads. So, this check will not work when
			 *  dynamic pool is enabled.
			 */

			zassert_false(k_thread_stack_is_valid(ptr[i], stack_size, K_USER),
				      "Dynamic stack is not valid");
#endif
		}
	}

	for (size_t i = 0; i < NUM_STACKS_TO_VALIDATE; i++) {
		if (ptr[i] != NULL) {
			zassert_equal(k_thread_stack_free(ptr[i]), 0,
				      "Freeing invalid stack memory");

			zassert_false(k_thread_stack_is_valid(ptr[i], stack_size, 0),
				      "Freed stack is not valid");
		}
	}

#ifdef CONFIG_USERSPACE
	/* Stack allocated for user must be valid for both kernel and user threads */
	for (size_t i = 0; i < NUM_STACKS_TO_VALIDATE; i++) {
		ptr[i] = k_thread_stack_alloc(stack_size, K_USER);
		if (ptr[i] != NULL) {
			/* Check if the stack is valid for kernel threads */
			zassert_true(k_thread_stack_is_valid(ptr[i], stack_size, 0),
				     "Dynamic stack is not valid");

			/* Check if the stack is valid for kernel threads */
			zassert_true(k_thread_stack_is_valid(ptr[i], stack_size, K_USER),
				     "Dynamic stack is not valid");
		}
	}

	for (size_t i = 0; i < NUM_STACKS_TO_VALIDATE; i++) {
		if (ptr[i] != NULL) {
			zassert_equal(k_thread_stack_free(ptr[i]), 0,
				      "Freeing invalid stack memory");
		}
	}
#endif /* CONFIG_USERSPACE */
}
#endif /* CONFIG_DYNAMIC_THREAD */

static void validate_static_stack(void)
{
	expect_fault = false;

	k_thread_create(&kernel_thread, kernel_thread_stack, TEST_THREADS_STACKSIZE,
			thread_function, NULL, NULL, NULL, -1, 0, K_NO_WAIT);

	k_thread_join(&kernel_thread, K_FOREVER);

	k_thread_create(&user_thread, user_thread_stack, TEST_THREADS_STACKSIZE, thread_function,
			NULL, NULL, NULL, -1, K_USER, K_NO_WAIT);

	k_thread_join(&user_thread, K_FOREVER);

	expect_fault = true;

	k_thread_create(&user_thread, (k_thread_stack_t *)user_thread_invalid_stack,
			TEST_THREADS_STACKSIZE, thread_function, NULL, NULL, NULL, -1, 0,
			K_NO_WAIT);

	/* Control should not come here and must assert in k_thread_create */
	expect_fault = false;

	k_thread_join(&user_thread, K_FOREVER);

	ztest_test_fail();
}

ZTEST(thread_stack_validate, test_thread_stack_validate)
{
#ifdef CONFIG_DYNAMIC_THREAD
	validate_dynamic_stack();
#endif /* CONFIG_DYNAMIC_THREAD */

	validate_static_stack();
}

ZTEST_SUITE(thread_stack_validate, NULL, NULL, NULL, NULL, NULL);
