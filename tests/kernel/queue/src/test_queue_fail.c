/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_queue.h"

#define TIMEOUT K_MSEC(100)
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define LIST_LEN 2

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

/*test cases*/
/**
 * @brief Test k_queue_get() failure scenario
 * @ingroup kernel_queue_tests
 * @see k_queue_get()
 */
void test_queue_get_fail(void)
{
	static struct k_queue queue;

	k_queue_init(&queue);
	/**TESTPOINT: queue get returns NULL*/
	zassert_is_null(k_queue_get(&queue, K_NO_WAIT), NULL);
	zassert_is_null(k_queue_get(&queue, TIMEOUT), NULL);
}

/* The sub-thread entry */
static void tThread_entry(void *p1, void *p2, void *p3)
{
	/* wait the queue for data */
	k_queue_get((struct k_queue *)p1, K_FOREVER);
}

/**
 * @brief Test k_queue_append_list() failure scenario
 *
 * @details Accroding to the API k_queue_append_list to
 * design some error condition to verify error branch of
 * the API.
 *	1. Verify that the list's head is empty.
 *	2. Verify that the list's tail is empty.
 *	3. Verify that append list to the queue when a
 * sub-thread is waiting for data.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_append_list()
 */
void test_queue_append_list_error(void)
{
	qdata_t data_l[2];
	static struct k_queue queue;
	qdata_t *head = NULL, *tail = &data_l[1];

	k_queue_init(&queue);
	memset(data_l, 0, sizeof(data_l));

	/* Check if the list of head is equal to null */
	zassert_true(k_queue_append_list(&queue, (uint32_t *)head,
			(uint32_t *)tail) == -EINVAL,
			"failed to CHECKIF head == NULL");
	/* Check if the list of tail is equal to null */
	head = &data_l[0];
	tail = NULL;
	zassert_true(k_queue_append_list(&queue, (uint32_t *)head,
			(uint32_t *)tail) == -EINVAL,
			"failed to CHECKIF tail == NULL");
	/* Initializing the queue for re-using below */
	k_queue_init(&queue);
	k_thread_create(&tdata, tstack, STACK_SIZE, tThread_entry, &queue,
			NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	/* Delay for thread initializing */
	k_sleep(K_MSEC(500));
	/* Append list into the queue for sub-thread */
	head = &data_l[0];
	tail = &data_l[1];
	head->snode.next = (sys_snode_t *)tail;
	tail->snode.next = NULL;
	k_queue_append_list(&queue, (uint32_t *)head, (uint32_t *)tail);
}

/**
 * @brief Test k_queue_merge_slist() failure scenario
 *
 * @details Verify the API k_queue_merge_slist when
 * a slist is empty or a slist's tail is null.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_merge_slist()
 */
void test_queue_merge_list_error(void)
{
	qdata_t data_sl[2];
	static struct k_queue queue;
	sys_slist_t slist;

	k_queue_init(&queue);
	sys_slist_init(&slist);
	memset(data_sl, 0, sizeof(data_sl));

	/* Check if the slist is empty */
	zassert_true(k_queue_merge_slist(&queue, &slist) == -EINVAL,
			"Failed to CHECKIF slist is empty");
	/* Check if the tail of the slist is null */
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[0].snode));
	sys_slist_append(&slist, (sys_snode_t *)&(data_sl[1].snode));
	slist.tail = NULL;
	zassert_true(k_queue_merge_slist(&queue, &slist) != 0,
			"Failed to CHECKIF the tail of slist == null");
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Test k_queue_init() failure scenario
 *
 * @details Verify that the parameter of API k_queue_init() is
 * NULL, what will happen.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_init()
 */
void test_queue_init_null(void)
{
	ztest_set_fault_valid(true);
	k_queue_init(NULL);
}

/**
 * @brief Test k_queue_alloc_append() failure scenario
 *
 * @details Verify that the parameter of the API is
 * NULL, what will happen.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_alloc_append()
 */
void test_queue_alloc_append_null(void)
{
	qdata_t data;

	memset(&data, 0, sizeof(data));
	ztest_set_fault_valid(true);
	k_queue_alloc_append(NULL, &data);
}

/**
 * @brief Test k_queue_alloc_prepend() failure scenario
 *
 * @details Verify that the parameter of the API is
 * NULL, what will happen.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_alloc_prepend()
 */
void test_queue_alloc_prepend_null(void)
{
	qdata_t data;

	memset(&data, 0, sizeof(data));
	ztest_set_fault_valid(true);
	k_queue_alloc_prepend(NULL, &data);
}

/**
 * @brief Test k_queue_get() failure scenario
 *
 * @details Verify that the parameter of the API is
 * NULL, what will happen.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_get()
 */
void test_queue_get_null(void)
{
	ztest_set_fault_valid(true);
	k_queue_get(NULL, K_FOREVER);
}

/**
 * @brief Test k_queue_is_empty() failure scenario
 *
 * @details Verify that the parameter of the API is
 * NULL, what will happen.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_is_empty()
 */
void test_queue_is_empty_null(void)
{
	ztest_set_fault_valid(true);
	k_queue_is_empty(NULL);
}

/**
 * @brief Test k_queue_peek_head() failure scenario
 *
 * @details Verify that the parameter of the API is
 * NULL, what will happen.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_peek_head()
 */
void test_queue_peek_head_null(void)
{
	ztest_set_fault_valid(true);
	k_queue_peek_head(NULL);
}

/**
 * @brief Test k_queue_peek_tail() failure scenario
 *
 * @details Verify that the parameter of the API is
 * NULL, what will happen.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_peek_tail()
 */
void test_queue_peek_tail_null(void)
{
	ztest_set_fault_valid(true);
	k_queue_peek_tail(NULL);
}

/**
 * @brief Test k_queue_merge_slist() failure scenario
 *
 * @details Verify that the parameter of the API is
 * NULL, what will happen.
 *
 * @ingroup kernel_queue_tests
 *
 * @see k_queue_merge_slist()
 */
void test_queue_cancel_wait_error(void)
{
	struct k_queue *q;

	q = k_object_alloc(K_OBJ_QUEUE);
	zassert_not_null(q, "no memory for allocated queue object");
	k_queue_init(q);

	/* Check if cancel a qeueu that no thread to wait */
	k_queue_cancel_wait(q);

	/* Check if cancel a null pointer */
	ztest_set_fault_valid(true);
	k_queue_cancel_wait(NULL);
}

#endif /* CONFIG_USERSPACE */
