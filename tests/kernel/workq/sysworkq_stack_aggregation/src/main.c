/*
 * Copyright (c) 2026 Priyanshu Singh <singhpriyanshu9838@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#if IS_ENABLED(CONFIG_TEST_EXPECT_USER_CONFIGURED)
#ifndef K_SYSTEM_WORKQUEUE_STACK_SIZE_USER_CONFIGURED
#error "Expected K_SYSTEM_WORKQUEUE_STACK_SIZE_USER_CONFIGURED to be defined"
#endif
#else
#ifdef K_SYSTEM_WORKQUEUE_STACK_SIZE_USER_CONFIGURED
#error "Did not expect K_SYSTEM_WORKQUEUE_STACK_SIZE_USER_CONFIGURED to be defined"
#endif
#endif

#if defined(CONFIG_TEST_EXPECTED_STACK_SIZE)
BUILD_ASSERT(K_SYSTEM_WORKQUEUE_STACK_SIZE == CONFIG_TEST_EXPECTED_STACK_SIZE,
	     "Compiled K_SYSTEM_WORKQUEUE_STACK_SIZE does not match expected size");
#endif

static struct k_work test_work;
static K_SEM_DEFINE(work_done_sem, 0, 1);

static void test_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_sem_give(&work_done_sem);
}

ZTEST(sysworkq_stack_aggregation, test_stack_size)
{
	k_tid_t tid = k_work_queue_thread_get(&k_sys_work_q);

	zassert_not_null(tid, "System workqueue thread ID must not be NULL");

#if defined(CONFIG_THREAD_STACK_INFO)
	size_t stack_size = tid->stack_info.size;

	TC_PRINT("Runtime sysworkq thread stack size: %zu bytes (compiled: %d, expected: %d)\n",
		 stack_size, K_SYSTEM_WORKQUEUE_STACK_SIZE, CONFIG_TEST_EXPECTED_STACK_SIZE);

	zassert_equal(stack_size, (size_t)K_SYSTEM_WORKQUEUE_STACK_SIZE,
		      "Thread stack size (%zu) does not match K_SYSTEM_WORKQUEUE_STACK_SIZE (%d)",
		      stack_size, K_SYSTEM_WORKQUEUE_STACK_SIZE);

#if defined(CONFIG_TEST_EXPECTED_STACK_SIZE)
	zassert_true(stack_size >= (size_t)CONFIG_TEST_EXPECTED_STACK_SIZE,
		     "Runtime thread stack size %zu is smaller than expected %d",
		     stack_size, CONFIG_TEST_EXPECTED_STACK_SIZE);
	zassert_true(stack_size < (size_t)CONFIG_TEST_EXPECTED_STACK_SIZE + 512U,
		     "Runtime thread stack size %zu is unexpectedly larger than expected %d",
		     stack_size, CONFIG_TEST_EXPECTED_STACK_SIZE);
#endif
#endif
}

ZTEST(sysworkq_stack_aggregation, test_execution)
{
	int ret;

	k_work_init(&test_work, test_work_handler);
	ret = k_work_submit_to_queue(&k_sys_work_q, &test_work);
	zassert_true(ret >= 0, "k_work_submit_to_queue failed: %d", ret);

	ret = k_sem_take(&work_done_sem, K_MSEC(1000));
	zassert_equal(ret, 0, "Timed out waiting for work item execution on sysworkq");
}

ZTEST(sysworkq_stack_aggregation, test_user_configured_flag)
{
#if IS_ENABLED(CONFIG_TEST_EXPECT_USER_CONFIGURED)
#if defined(K_SYSTEM_WORKQUEUE_STACK_SIZE_USER_CONFIGURED)
	zassert_true(true, "User configured flag correctly set");
#else
	zassert_unreachable("User configured flag was expected but not defined");
#endif
#else
#if defined(K_SYSTEM_WORKQUEUE_STACK_SIZE_USER_CONFIGURED)
	zassert_unreachable("User configured flag was not expected but was defined");
#else
	zassert_true(true, "Default unconfigured state correctly verified");
#endif
#endif
}

ZTEST_SUITE(sysworkq_stack_aggregation, NULL, NULL, NULL, NULL, NULL);
