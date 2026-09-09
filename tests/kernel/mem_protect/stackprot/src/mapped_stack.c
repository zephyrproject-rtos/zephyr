/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel/thread_stack.h>

#ifdef CONFIG_THREAD_STACK_MEM_MAPPED

#define STACK_SIZE (CONFIG_MMU_PAGE_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE)

K_THREAD_STACK_DEFINE(mapped_thread_stack_area, STACK_SIZE);
static struct k_thread mapped_thread_data;

ZTEST_BMEM static char *mapped_stack_addr;
ZTEST_BMEM static size_t mapped_stack_sz;

/**
 * @brief To cause fault in guard pages.
 *
 * @param p1 0 if testing rear guard page, 1 if testing front guard page.
 * @param p2 Unused.
 * @param p3 Unused.
 */
void mapped_thread(void *p1, void *p2, void *p3)
{
	bool is_front = p1 == NULL ? false : true;
	volatile char *ptr;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	TC_PRINT("Starts %s\n", __func__);
	TC_PRINT("Mapped stack %p size %zu\n", mapped_stack_addr, mapped_stack_sz);

	ptr = mapped_stack_addr;

	/* Figure out where to cause the stack fault. */
	if (is_front) {
		/* Middle of front guard page. */
		ptr -= CONFIG_MMU_PAGE_SIZE / 2;
	} else {
		/* Middle of rear guard page. */
		ptr += mapped_stack_sz;
		ptr += CONFIG_MMU_PAGE_SIZE / 2;
	}

	TC_PRINT("Trying to cause stack fault at %p\n", ptr);

	*ptr = 0;

	TC_PRINT("Should have fault on guard page but not!\n");
	ztest_test_fail();
}

/**
 * @brief To create thread to fault on guard pages.
 *
 * @param is_front True if testing front guard page, false if testing rear guard page.
 * @param is_user True if creating a user thread, false if creating a kernel thread.
 */
void create_thread(bool is_front, bool is_user)
{
	/* Start thread */
	k_thread_create(&mapped_thread_data, mapped_thread_stack_area, STACK_SIZE,
			mapped_thread,
			is_front ? (void *)1 : (void *)0,
			NULL, NULL,
			K_PRIO_COOP(1), is_user ? K_USER : 0, K_FOREVER);

	zassert_true(mapped_thread_data.stack_info.mapped.addr != NULL);

	/* Grab the mapped stack object address and size so we can calculate
	 * where to cause a stack fault.
	 */
	mapped_stack_addr = (void *)mapped_thread_data.stack_info.mapped.addr;
	mapped_stack_sz = mapped_thread_data.stack_info.mapped.sz;

	k_thread_start(&mapped_thread_data);

	/* Note that this sleep is required on SMP platforms where
	 * that thread will execute asynchronously!
	 */
	k_sleep(K_MSEC(100));

	k_thread_join(&mapped_thread_data, K_FOREVER);
}

#endif /* CONFIG_THREAD_STACK_MEM_MAPPED */

/**
 * @brief Verify that the front guard page of a mapped stack faults.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * A memory-mapped thread stack is surrounded by unmapped guard pages, so an
 * access that runs off either end of the stack faults immediately instead of
 * corrupting whatever happens to be adjacent. The spawned thread deliberately
 * touches the middle of the front guard page; the fault is the expected outcome,
 * and running past the access fails the test. Skipped when
 * CONFIG_THREAD_STACK_MEM_MAPPED is not enabled.
 * Test steps:
 * - Create a kernel thread on a memory-mapped stack and record the mapped
 *   stack's address and size.
 * - In the thread, write to the middle of the front guard page.
 *
 * Expected result:
 * - The write faults on the guard page and the thread is terminated; the code
 *   after the write is never reached.
 */
ZTEST(stackprot_mapped_stack, test_stackprot_guard_page_front)
{
#ifdef CONFIG_THREAD_STACK_MEM_MAPPED
	create_thread(true, false);
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Verify that the rear guard page of a mapped stack faults.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * A memory-mapped thread stack is surrounded by unmapped guard pages, so an
 * access that runs off either end of the stack faults immediately instead of
 * corrupting whatever happens to be adjacent. The spawned thread deliberately
 * touches the middle of the rear guard page; the fault is the expected outcome,
 * and running past the access fails the test. Skipped when
 * CONFIG_THREAD_STACK_MEM_MAPPED is not enabled.
 * Test steps:
 * - Create a kernel thread on a memory-mapped stack and record the mapped
 *   stack's address and size.
 * - In the thread, write to the middle of the rear guard page.
 *
 * Expected result:
 * - The write faults on the guard page and the thread is terminated; the code
 *   after the write is never reached.
 */
ZTEST(stackprot_mapped_stack, test_stackprot_guard_page_rear)
{
#ifdef CONFIG_THREAD_STACK_MEM_MAPPED
	create_thread(false, false);
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Verify that the front guard page faults for a user thread.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * A memory-mapped thread stack is surrounded by unmapped guard pages, so an
 * access that runs off either end of the stack faults immediately instead of
 * corrupting whatever happens to be adjacent. The spawned thread deliberately
 * touches the middle of the front guard page; the fault is the expected outcome,
 * and running past the access fails the test. Skipped when
 * CONFIG_THREAD_STACK_MEM_MAPPED is not enabled.
 * Test steps:
 * - Create a user thread on a memory-mapped stack and record the mapped
 *   stack's address and size.
 * - In the thread, write to the middle of the front guard page.
 *
 * Expected result:
 * - The write faults on the guard page and the thread is terminated; the code
 *   after the write is never reached.
 */
ZTEST(stackprot_mapped_stack, test_stackprot_guard_page_front_user)
{
#ifdef CONFIG_THREAD_STACK_MEM_MAPPED
	create_thread(true, true);
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Verify that the rear guard page faults for a user thread.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * A memory-mapped thread stack is surrounded by unmapped guard pages, so an
 * access that runs off either end of the stack faults immediately instead of
 * corrupting whatever happens to be adjacent. The spawned thread deliberately
 * touches the middle of the rear guard page; the fault is the expected outcome,
 * and running past the access fails the test. Skipped when
 * CONFIG_THREAD_STACK_MEM_MAPPED is not enabled.
 * Test steps:
 * - Create a user thread on a memory-mapped stack and record the mapped
 *   stack's address and size.
 * - In the thread, write to the middle of the rear guard page.
 *
 * Expected result:
 * - The write faults on the guard page and the thread is terminated; the code
 *   after the write is never reached.
 */
ZTEST(stackprot_mapped_stack, test_stackprot_guard_page_rear_user)
{
#ifdef CONFIG_THREAD_STACK_MEM_MAPPED
	create_thread(false, true);
#else
	ztest_test_skip();
#endif
}


ZTEST_SUITE(stackprot_mapped_stack, NULL, NULL, NULL, NULL, NULL);
