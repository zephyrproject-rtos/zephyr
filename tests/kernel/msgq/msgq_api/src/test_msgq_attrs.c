/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_msgq.h"
extern struct k_msgq msgq;
static ZTEST_BMEM char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static ZTEST_DMEM uint32_t send_buf[MSGQ_LEN] = { MSG0, MSG1 };
static ZTEST_DMEM uint32_t rec_buf[MSGQ_LEN] = { MSG0, MSG1 };

static void attrs_get(struct k_msgq *q)
{
	int ret;
	struct k_msgq_attrs attrs;

	k_msgq_get_attrs(q, &attrs);
	zassert_equal(attrs.used_msgs, 0);
	zassert_equal(k_msgq_num_used_get(q), 0);
	zassert_equal(k_msgq_num_free_get(q), MSGQ_LEN);

	/*fill the queue to full*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(q, (void *)&send_buf[i], K_NO_WAIT);
		zassert_equal(ret, 0);
	}

	k_msgq_get_attrs(q, &attrs);
	zassert_equal(attrs.used_msgs, MSGQ_LEN);
	zassert_equal(k_msgq_num_used_get(q), MSGQ_LEN);
	zassert_equal(k_msgq_num_free_get(q), 0);

	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_get(q, (void *)&rec_buf[i], K_NO_WAIT);
		zassert_equal(ret, 0);
	}

	k_msgq_get_attrs(q, &attrs);
	zassert_equal(attrs.used_msgs, 0);
	zassert_equal(k_msgq_num_used_get(q), 0);
	zassert_equal(k_msgq_num_free_get(q), MSGQ_LEN);
}

/**
 * @addtogroup tests_kernel_msgq
 * @{
 */

/**
 * @brief Verify the used and free message count queries.
 *
 * @details
 * k_msgq_get_attrs(), k_msgq_num_used_get() and k_msgq_num_free_get() must
 * all reflect the live occupancy of the queue. The test reads them when
 * empty, after filling the queue to capacity, and again after draining it,
 * confirming the counts track each transition and agree with each other.
 *
 * Test steps:
 * - Query an empty queue and verify 0 used and all slots free.
 * - Fill the queue and verify the counts report it as full.
 * - Drain the queue and verify the empty-queue counts again.
 *
 * Expected result:
 * - The used and free counts report empty, full, then empty as messages are
 *   added and removed, through the attributes and the direct getters alike.
 *
 * @see k_msgq_get_attrs()
 * @see k_msgq_num_used_get()
 * @see k_msgq_num_free_get()
 * @verifies ZEP-SRS-31-15
 * @verifies ZEP-SRS-31-16
 * @verifies ZEP-SRS-31-21
 */
ZTEST(msgq_api, test_msgq_attrs_get)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	attrs_get(&msgq);
}

#ifdef CONFIG_USERSPACE

/**
 * @brief Verify the message count queries from user mode on an allocated queue.
 *
 * @details
 * Same used and free message accounting as test_msgq_attrs_get(), but
 * exercised from a user-mode thread on a queue created with
 * k_msgq_alloc_init(), confirming the count queries work under userspace with
 * a dynamically allocated queue.
 *
 * Test steps:
 * - Allocate and alloc-init a message queue as a user thread.
 * - Verify the counts report empty, then full, then empty across fill/drain.
 *
 * Expected result:
 * - The used and free counts are reported correctly from user mode.
 *
 * @see k_msgq_get_attrs()
 * @see k_msgq_num_used_get()
 * @see k_msgq_num_free_get()
 * @see k_msgq_alloc_init()
 * @verifies ZEP-SRS-31-15
 * @verifies ZEP-SRS-31-16
 * @verifies ZEP-SRS-31-21
 */
ZTEST_USER(msgq_api, test_msgq_user_attrs_get)
{
	struct k_msgq *q;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN));
	attrs_get(q);
}
#endif

/**
 * @}
 */
