/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#if defined(CONFIG_WORKQUEUE_WORK_TIMEOUT)
#include <kernel_internal.h>
#endif

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

#if defined(CONFIG_WORKQUEUE_WORK_TIMEOUT)
/* Fires from ISR context. Report the verdict and halt here directly. */
void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	int rv = (reason == K_ERR_WORK_TIMEOUT) ? TC_PASS : TC_FAIL;

	ARG_UNUSED(esf);

	TC_PRINT("work timeout fatal handler: reason %d\n", reason);
	TC_END_RESULT_CUSTOM(rv, "workqueue_work_timeout_test_workq_work_timeout");
	TC_END_REPORT(rv);
	arch_system_halt(reason);
}
#endif /* CONFIG_WORKQUEUE_WORK_TIMEOUT */

static void *test_setup(void)
{
	const struct k_work_queue_config config = {
		.name = "sysworkq",
		.no_yield = false,
		.essential = false,
		.work_timeout_ms = TEST_WORK_TIMEOUT_MS,
	};

	k_work_queue_start(&test_workq, test_workq_stack, K_KERNEL_STACK_SIZEOF(test_workq_stack),
			   0, &config);

	return NULL;
}

ZTEST_SUITE(workqueue_work_timeout, NULL, test_setup, NULL, NULL, NULL);

/**
 * @brief Verify a work queue thread is aborted when a work item exceeds the
 * configured timeout.
 *
 * @details
 * A work queue configured with a work timeout monitors how long each work item
 * handler runs. This test submits several short items (which must not trip the
 * timeout) followed by a blocking item that runs forever, and confirms the work
 * queue thread is aborted only when work timeout monitoring is enabled.
 *
 * Test steps:
 * - Submit several work items that each run for less than the timeout.
 * - Confirm the work queue thread is not aborted while processing them.
 * - Submit a work item whose handler blocks forever.
 * - With CONFIG_WORKQUEUE_WORK_TIMEOUT disabled, join the work queue thread.
 * - With CONFIG_WORKQUEUE_WORK_TIMEOUT enabled, wait past the timeout instead:
 *   the resulting fatal error reports the test verdict and halts from
 *   k_sys_fatal_error_handler() (see above), so this function never returns
 *   in that configuration.
 *
 * Expected result:
 * - With CONFIG_WORKQUEUE_WORK_TIMEOUT disabled, the join times out with
 *   -EAGAIN (the thread is never aborted).
 * - With CONFIG_WORKQUEUE_WORK_TIMEOUT enabled, the thread is aborted and a
 *   fatal error with reason K_ERR_WORK_TIMEOUT is raised; the test verdict is
 *   reported from k_sys_fatal_error_handler() instead of this function.
 *
 * @see k_work_queue_start()
 * @see k_work_submit_to_queue()
 * @ingroup kernel_workqueue_tests
 */
ZTEST(workqueue_work_timeout, test_workq_work_timeout)
{
	/* Submit multiple items which take less time than TEST_WORK_TIMEOUT_MS each */
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work0), 1);
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work1), 1);
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work2), 1);
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work3), 1);

	/*
	 * Submitted items takes longer than TEST_WORK_TIMEOUT_MS, but each item takes
	 * less time than TEST_WORK_DELAY so workqueue thread will not be aborted.
	 */
	zassert_equal(k_thread_join(test_workq.thread_id, TEST_WORK_DELAY), -EAGAIN);

	/* Submit single item which takes longer than TEST_WORK_TIMEOUT_MS */
	zassert_equal(k_work_submit_to_queue(&test_workq, &test_work_blocking), 1);

	if (IS_ENABLED(CONFIG_WORKQUEUE_WORK_TIMEOUT)) {
		/*
		 * The blocking item's timeout raises a fatal error from ISR
		 * context. k_sys_fatal_error_handler() above reports the verdict
		 * and halts directly; control does not return here. If it does,
		 * the timeout path did not fire as expected.
		 */
		k_sleep(TEST_WORK_BLOCKING_DELAY);
		zassert_unreachable("work timeout did not raise a fatal error");
	} else {
		zassert_equal(k_thread_join(test_workq.thread_id, TEST_WORK_BLOCKING_DELAY),
			      -EAGAIN);
	}
}
