/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "test_msgq.h"

static char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static u32_t data[MSGQ_LEN] = { MSG0, MSG1 };
extern struct k_msgq msgq;

static void put_fail(struct k_msgq *q)
{
	int ret = k_msgq_put(q, (void *)&data[0], K_NO_WAIT);

	zassert_false(ret, NULL);
	ret = k_msgq_put(q, (void *)&data[0], K_NO_WAIT);
	zassert_false(ret, NULL);
	/**TESTPOINT: msgq put returns -ENOMSG*/
	ret = k_msgq_put(q, (void *)&data[1], K_NO_WAIT);
	zassert_equal(ret, -ENOMSG, NULL);
	/**TESTPOINT: msgq put returns -EAGAIN*/
	ret = k_msgq_put(q, (void *)&data[0], TIMEOUT);
	zassert_equal(ret, -EAGAIN, NULL);

	k_msgq_purge(q);
}

static void get_fail(struct k_msgq *q)
{
	u32_t rx_data;

	/**TESTPOINT: msgq get returns -ENOMSG*/
	int ret = k_msgq_get(q, &rx_data, K_NO_WAIT);

	zassert_equal(ret, -ENOMSG, NULL);
	/**TESTPOINT: msgq get returns -EAGAIN*/
	ret = k_msgq_get(q, &rx_data, TIMEOUT);
	zassert_equal(ret, -EAGAIN, NULL);
}

/**
 * @addtogroup kernel_message_queue_tests
 * @{
 */

/**
 * @brief Test returned error code during writing in msgq
 * @see k_msgq_init()
 */
void test_msgq_put_fail(void)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	put_fail(&msgq);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Test returned error code during writing in msgq
 * @see k_msgq_alloc_init()
 */
void test_msgq_user_put_fail(void)
{
	struct k_msgq *q;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN), NULL);
	put_fail(q);
}
#endif /* CONFIG_USERSPACE */

/**
 * @brief Test returned error code during reading from msgq
 * @see k_msgq_init(), k_msgq_put()
 */
void test_msgq_get_fail(void)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	get_fail(&msgq);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Test returned error code during reading from msgq
 * @see k_msgq_alloc_init(), k_msgq_get()
 */
void test_msgq_user_get_fail(void)
{
	struct k_msgq *q;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN), NULL);
	get_fail(q);
}
#endif /* CONFIG_USERSPACE */

/**
 * @}
 */
