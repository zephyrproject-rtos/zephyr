/*
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Workqueue Tests
 * @defgroup kernel_workqueue_tests Workqueue
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define NUM_OF_WORK 2
#define SYNC_SEM_INIT_VAL (0U)

static K_THREAD_STACK_DEFINE(user_tstack, STACK_SIZE);
static struct k_work_user_q user_workq;
static ZTEST_BMEM struct k_work_user work[NUM_OF_WORK];
static struct k_sem sync_sema;
static struct k_sem dummy_sema;
static struct k_thread *main_thread;

/**
 * @brief Common function using like a handler for workqueue tests
 * API call in it means successful execution of that function
 *
 * @param unused of type k_work to make handler function accepted
 * by k_work_init
 *
 * @return N/A
 */
static void common_work_handler(struct k_work_user *unused)
{
	k_sem_give(&sync_sema);
}

static void test_k_work_user_init(void)
{
	K_WORK_USER_DEFINE(local, common_work_handler);
	zassert_equal(local.handler, common_work_handler, NULL);
	zassert_equal(local.flags, 0, NULL);
}

/**
 * @brief Test k_work_user_submit_to_queue API
 *
 * @details Function k_work_user_submit_to_queue() will return
 * -EBUSY: if the work item was already in some workqueue and
 * -ENOMEM: if no memory for thread resource pool allocation.
 * Create two situation to meet the error return value.
 *
 * @see k_work_user_submit_to_queue()
 * @ingroup kernel_workqueue_tests
 */
static void test_k_work_user_submit_to_queue_fail(void)
{
	int ret = 0;

	k_sem_reset(&sync_sema);
	k_work_user_init(&work[0], common_work_handler);
	k_work_user_init(&work[1], common_work_handler);

	/* TESTPOINT: When a work item be added to a workqueue, its flag will
	 * be in pending state, before the work item be processed, it cannot
	 * be append to a workqueue another time.
	 */
	k_work_user_submit_to_queue(&user_workq, &work[0]);
	zassert_true(k_work_user_is_pending(&work[0]), NULL);
	k_work_user_submit_to_queue(&user_workq, &work[0]);

	/* Test the work item's callback function can only be invoked once */
	k_sem_take(&sync_sema, K_FOREVER);
	zassert_true(k_queue_is_empty(&user_workq.queue), NULL);
	zassert_false(k_work_user_is_pending(&work[0]), NULL);

	/* use up the memory in resource pool */
	for (int i = 0; i < 100; i++) {
		ret = k_queue_alloc_append(&user_workq.queue, &work[1]);
		if (ret == -ENOMEM) {
			break;
		}
	}

	k_work_user_submit_to_queue(&user_workq, &work[0]);
	/* if memory is used up, the work cannot be append into the workqueue */
	zassert_false(k_work_user_is_pending(&work[0]), NULL);
}


static void work_handler(struct k_work_user *w)
{
	/* Just to show an API call on this will succeed */
	k_sem_init(&dummy_sema, 0, 1);

	k_sem_give(&sync_sema);
}

static void twork_submit_1(struct k_work_user_q *work_q, struct k_work_user *w,
			   k_work_user_handler_t handler)
{
	/**TESTPOINT: init via k_work_init*/
	k_work_user_init(w, handler);
	/**TESTPOINT: check pending after work init*/
	zassert_false(k_work_user_is_pending(w), NULL);

	/**TESTPOINT: work submit to queue*/
	zassert_false(k_work_user_submit_to_queue(work_q, w),
		      "failed to submit to queue");
}

static void twork_submit(const void *data)
{
	struct k_work_user_q *work_q = (struct k_work_user_q *)data;

	for (int i = 0; i < NUM_OF_WORK; i++) {
		twork_submit_1(work_q, &work[i], work_handler);
	}
}

/**
 * @brief Test user mode work queue start before submit
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_q_user_start()
 */
static void test_work_user_queue_start_before_submit(void)
{
	k_work_user_queue_start(&user_workq, user_tstack, STACK_SIZE,
				CONFIG_MAIN_THREAD_PRIORITY, "user.wq");
}

/**
 * @brief Setup object permissions before test_user_workq_granted_access()
 *
 * @ingroup kernel_workqueue_tests
 */
static void test_user_workq_granted_access_setup(void)
{
	/* Subsequent test cases will have access to the dummy_sema,
	 * but not the user workqueue since it already started.
	 */
	k_object_access_grant(&dummy_sema, main_thread);
}

/**
 * @brief Test user mode grant workqueue permissions
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_q_object_access_grant()
 */
static void test_user_workq_granted_access(void)
{
	k_object_access_grant(&dummy_sema, &user_workq.thread);
}

/**
 * @brief Test work submission to work queue (user mode)
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_init(), k_work_is_pending(), k_work_submit_to_queue(),
 * k_work_submit()
 */
static void test_user_work_submit_to_queue_thread(void)
{
	k_sem_reset(&sync_sema);
	twork_submit(&user_workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

void test_main(void)
{
	main_thread = k_current_get();
	k_thread_access_grant(main_thread, &sync_sema, &user_workq.thread,
			      &user_workq.queue,
			      &user_tstack);
	k_sem_init(&sync_sema, SYNC_SEM_INIT_VAL, NUM_OF_WORK);
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(workqueue_api,
			 /* Do not disturb the ordering of these test cases */
			 ztest_user_unit_test(test_work_user_queue_start_before_submit),
			 ztest_unit_test(test_user_workq_granted_access_setup),
			 ztest_user_unit_test(test_user_workq_granted_access),
			 /* End order-important tests */
			 ztest_unit_test(test_k_work_user_init),
			 ztest_1cpu_user_unit_test(test_user_work_submit_to_queue_thread),
			 ztest_user_unit_test(test_k_work_user_submit_to_queue_fail)
		);

	ztest_run_test_suite(workqueue_api);
}
