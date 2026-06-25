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

	/*fill the queue to full*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(q, (void *)&send_buf[i], K_NO_WAIT);
		zassert_equal(ret, 0);
	}

	k_msgq_get_attrs(q, &attrs);
	zassert_equal(attrs.used_msgs, MSGQ_LEN);

	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_get(q, (void *)&rec_buf[i], K_NO_WAIT);
		zassert_equal(ret, 0);
	}

	k_msgq_get_attrs(q, &attrs);
	zassert_equal(attrs.used_msgs, 0);
}

/**
 * @addtogroup tests_kernel_msgq
 * @{
 */

/**
 * @brief Verify k_msgq_get_attrs() reports the used-message count.
 *
 * @details
 * k_msgq_get_attrs() must reflect the live occupancy of the queue. The test
 * reads the attributes when empty, after filling the queue to capacity, and
 * again after draining it, confirming used_msgs tracks each transition.
 *
 * Test steps:
 * - Get attributes of an empty queue and verify used_msgs is 0.
 * - Fill the queue and verify used_msgs equals the queue length.
 * - Drain the queue and verify used_msgs is 0 again.
 *
 * Expected result:
 * - used_msgs reports 0, full, then 0 as messages are added and removed.
 *
 * @see k_msgq_get_attrs()
 * @verifies ZEP-SRS-31-15
 * @verifies ZEP-SRS-31-16
 */
ZTEST(msgq_api, test_msgq_attrs_get)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	attrs_get(&msgq);
}

#ifdef CONFIG_USERSPACE

/**
 * @brief Verify k_msgq_get_attrs() from user mode on an allocated queue.
 *
 * @details
 * Same used-message accounting as test_msgq_attrs_get(), but exercised from a
 * user-mode thread on a queue created with k_msgq_alloc_init(), confirming the
 * attribute query works under userspace with a dynamically allocated queue.
 *
 * Test steps:
 * - Allocate and alloc-init a message queue as a user thread.
 * - Verify used_msgs is 0, then full, then 0 across fill/drain.
 *
 * Expected result:
 * - used_msgs is reported correctly from user mode.
 *
 * @see k_msgq_get_attrs()
 * @see k_msgq_alloc_init()
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
