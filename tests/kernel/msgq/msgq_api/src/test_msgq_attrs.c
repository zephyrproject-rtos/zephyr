/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_msgq.h"
extern struct k_msgq msgq;
static ZTEST_BMEM char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static ZTEST_DMEM u32_t send_buf[MSGQ_LEN] = { MSG0, MSG1 };
static ZTEST_DMEM u32_t rec_buf[MSGQ_LEN] = { MSG0, MSG1 };

static void attrs_get(struct k_msgq *q)
{
	int ret;
	struct k_msgq_attrs attrs;

	k_msgq_get_attrs(q, &attrs);
	zassert_equal(attrs.used_msgs, 0, NULL);

	/*fill the queue to full*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(q, (void *)&send_buf[i], K_NO_WAIT);
		zassert_equal(ret, 0, NULL);
	}

	k_msgq_get_attrs(q, &attrs);
	zassert_equal(attrs.used_msgs, MSGQ_LEN, NULL);

	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_get(q, (void *)&rec_buf[i], K_NO_WAIT);
		zassert_equal(ret, 0, NULL);
	}

	k_msgq_get_attrs(q, &attrs);
	zassert_equal(attrs.used_msgs, 0, NULL);
}

/**
 * @addtogroup kernel_message_queue_tests
 * @{
 */

/**
 * @brief Test basic attributes of a message queue
 *
 * @see  k_msgq_get_attrs()
 */
void test_msgq_attrs_get(void)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	attrs_get(&msgq);
}

#ifdef CONFIG_USERSPACE

/**
 * @brief Test basic attributes of a message queue
 *
 * @see  k_msgq_get_attrs()
 */
void test_msgq_user_attrs_get(void)
{
	struct k_msgq *q;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN), NULL);
	attrs_get(q);
}
#endif

/**
 * @}
 */
