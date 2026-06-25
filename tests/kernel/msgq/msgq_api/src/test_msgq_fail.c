/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "test_msgq.h"

static ZTEST_BMEM char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static ZTEST_DMEM uint32_t data[MSGQ_LEN] = { MSG0, MSG1 };
extern struct k_msgq msgq;

static void put_fail(struct k_msgq *q)
{
	int ret = k_msgq_put(q, (void *)&data[0], K_NO_WAIT);

	zassert_false(ret);
	ret = k_msgq_put(q, (void *)&data[0], K_NO_WAIT);
	zassert_false(ret);
	/**TESTPOINT: msgq put returns -ENOMSG*/
	ret = k_msgq_put(q, (void *)&data[1], K_NO_WAIT);
	zassert_equal(ret, -ENOMSG);
	/**TESTPOINT: msgq put returns -EAGAIN*/
	ret = k_msgq_put(q, (void *)&data[0], TIMEOUT);
	zassert_equal(ret, -EAGAIN);

	k_msgq_purge(q);
}

static void get_fail(struct k_msgq *q)
{
	uint32_t rx_data;

	/**TESTPOINT: msgq get returns -ENOMSG*/
	int ret = k_msgq_get(q, &rx_data, K_NO_WAIT);

	zassert_equal(ret, -ENOMSG);
	/**TESTPOINT: msgq get returns -EAGAIN*/
	ret = k_msgq_get(q, &rx_data, TIMEOUT);
	zassert_equal(ret, -EAGAIN);
}

/**
 * @addtogroup tests_kernel_msgq
 * @{
 */

/**
 * @brief Verify k_msgq_put() error codes when the queue is full.
 *
 * @details
 * Once a queue is full, a non-blocking put must return -ENOMSG and a put with a
 * finite timeout must return -EAGAIN after the timeout elapses.
 *
 * Test steps:
 * - Fill the queue, then put with K_NO_WAIT and expect -ENOMSG.
 * - Put with a finite timeout and expect -EAGAIN.
 *
 * Expected result:
 * - k_msgq_put() returns -ENOMSG (no wait) and -EAGAIN (timeout) when full.
 *
 * @see k_msgq_put()
 * @verifies ZEP-SRS-31-7
 */
ZTEST(msgq_api_1cpu, test_msgq_put_fail)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	put_fail(&msgq);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Verify k_msgq_put() full-queue error codes from user mode.
 *
 * @details
 * User-mode counterpart of test_msgq_put_fail() on a queue created with
 * k_msgq_alloc_init(): a full-queue put must return -ENOMSG (no wait) and
 * -EAGAIN (timeout).
 *
 * Test steps:
 * - Allocate and alloc-init a queue as a user thread and fill it.
 * - Put with K_NO_WAIT (-ENOMSG) and with a finite timeout (-EAGAIN).
 *
 * Expected result:
 * - The error codes match the supervisor case from user mode.
 *
 * @see k_msgq_put()
 * @see k_msgq_alloc_init()
 */
ZTEST_USER(msgq_api, test_msgq_user_put_fail)
{
	struct k_msgq *q;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN));
	put_fail(q);
}
#endif /* CONFIG_USERSPACE */

/**
 * @brief Verify k_msgq_get() error codes when the queue is empty.
 *
 * @details
 * On an empty queue, a non-blocking get must return -ENOMSG and a get with a
 * finite timeout must return -EAGAIN after the timeout elapses.
 *
 * Test steps:
 * - On an empty queue, get with K_NO_WAIT and expect -ENOMSG.
 * - Get with a finite timeout and expect -EAGAIN.
 *
 * Expected result:
 * - k_msgq_get() returns -ENOMSG (no wait) and -EAGAIN (timeout) when empty.
 *
 * @see k_msgq_get()
 * @verifies ZEP-SRS-31-11
 */
ZTEST(msgq_api_1cpu, test_msgq_get_fail)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	get_fail(&msgq);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Verify k_msgq_get() empty-queue error codes from user mode.
 *
 * @details
 * User-mode counterpart of test_msgq_get_fail() on a queue created with
 * k_msgq_alloc_init(): an empty-queue get must return -ENOMSG (no wait) and
 * -EAGAIN (timeout).
 *
 * Test steps:
 * - Allocate and alloc-init a queue as a user thread.
 * - Get with K_NO_WAIT (-ENOMSG) and with a finite timeout (-EAGAIN).
 *
 * Expected result:
 * - The error codes match the supervisor case from user mode.
 *
 * @see k_msgq_get()
 * @see k_msgq_alloc_init()
 */
ZTEST_USER(msgq_api, test_msgq_user_get_fail)
{
	struct k_msgq *q;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN));
	get_fail(q);
}
#endif /* CONFIG_USERSPACE */

/**
 * @}
 */
