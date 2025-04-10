/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_WORK_TIMEOUT_MS     100
#define TEST_WORK_DURATION_MS    (TEST_WORK_TIMEOUT_MS / 2)
#define TEST_WORK_DELAY          K_MSEC(TEST_WORK_DURATION_MS * 6)
#define TEST_WORK_BLOCKING_DELAY K_MSEC(TEST_WORK_TIMEOUT_MS * 2)

static struct k_work_q test_workq;
static K_KERNEL_STACK_DEFINE(test_workq_stack, CONFIG_MAIN_STACK_SIZE);

static void test_work_handler(struct k_work *work)
{
	k_msleep(TEST_WORK_DURATION_MS);
}

static K_WORK_DEFINE(test_work0, test_work_handler);
static K_WORK_DEFINE(test_work1, test_work_handler);
static K_WORK_DEFINE(test_work2, test_work_handler);
static K_WORK_DEFINE(test_work3, test_work_handler);

static void test_work_handler_blocking(struct k_work *work)
{
	k_sleep(K_FOREVER);
}

static K_WORK_DEFINE(test_work_blocking, test_work_handler_blocking);

static void *test_setup(void)
{
	const struct k_work_queue_config config = {
		.name = "sysworkq",
		.no_yield = false,
		.essential = false,
		.work_timeout_ms = TEST_WORK_TIMEOUT_MS,
	};

	k_work_queue_start(&test_workq,
			   test_workq_stack,
			   K_KERNEL_STACK_SIZEOF(test_workq_stack),
			   0,
			   &config);

	return NULL;
}

ZTEST_SUITE(workqueue_work_timeout, NULL, test_setup, NULL, NULL, NULL);

ZTEST(workqueue_work_timeout, test_work)
{
	int ret;

	/* Submit multiple items which take less time than TEST_WORK_TIMEOUT_MS each */
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work0), 1);
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work1), 1);
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work2), 1);
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work3), 1);

	/*
	 * Submitted items takes longer than TEST_WORK_TIMEOUT_MS, but each item takes
	 * less time than TEST_WORK_DELAY so workqueue thread will not be aborted.
	 */
	zassert_equal(k_thread_join(&test_workq.thread, TEST_WORK_DELAY), -EAGAIN);

	/* Submit single item which takes longer than TEST_WORK_TIMEOUT_MS */
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work_blocking), 1);

	/*
	 * Submitted item shall cause the work to time out and the workqueue thread be
	 * aborted if CONFIG_WORKQUEUE_WORK_TIMEOUT is enabled.
	 */
	ret = k_thread_join(&test_workq.thread, TEST_WORK_BLOCKING_DELAY);
	if (IS_ENABLED(CONFIG_WORKQUEUE_WORK_TIMEOUT)) {
		zassert_equal(ret, 0);
	} else {
		zassert_equal(ret, -EAGAIN);
	}
}
