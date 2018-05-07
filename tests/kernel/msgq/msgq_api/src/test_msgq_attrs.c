/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_msgq.h"
extern struct k_msgq msgq;
static char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static u32_t send_buf[MSGQ_LEN] = { MSG0, MSG1 };
static u32_t rec_buf[MSGQ_LEN] = { MSG0, MSG1 };

/**
 * @brief Verify zephyr msgq get attributes API.
 * @addtogroup kernel_message_queue
 * @{
 */
void test_msgq_attrs_get(void)
{
	int ret;
	struct k_msgq_attrs attrs;

	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);

	k_msgq_get_attrs(&msgq, &attrs);
	zassert_equal(attrs.used_msgs, 0, NULL);

	/*fill the queue to full*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(&msgq, (void *)&send_buf[i], K_NO_WAIT);
		zassert_equal(ret, 0, NULL);
	}

	k_msgq_get_attrs(&msgq, &attrs);
	zassert_equal(attrs.used_msgs, MSGQ_LEN, NULL);

	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_get(&msgq, (void *)&rec_buf[i], K_NO_WAIT);
		zassert_equal(ret, 0, NULL);
	}

	k_msgq_get_attrs(&msgq, &attrs);
	zassert_equal(attrs.used_msgs, 0, NULL);
}
/**
 * @}
 */
