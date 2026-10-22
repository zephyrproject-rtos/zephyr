/*
 * Copyright (c) 2026 Cesar Vandevelde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "test_msgq.h"

struct typed_msgq_msg {
	uint8_t num1;
	uint64_t num2;
};

K_MSGQ_DEFINE(kmsgq_public, MSG_SIZE, MSGQ_LEN, 4);
K_MSGQ_DEFINE_STATIC(kmsgq_static, MSG_SIZE, MSGQ_LEN, 4);
K_MSGQ_DEFINE_TYPE(kmsgq_type, struct typed_msgq_msg, MSGQ_LEN);
K_MSGQ_DEFINE_STATIC_TYPE(kmsgq_static_type, struct typed_msgq_msg, MSGQ_LEN);

/**
 * @brief Test public-scope message queue defined at compile time
 *
 * @details
 * - Verify that the free count is equal to the queue length.
 * - Verify that the used count equals 0.
 * - Verify that the message size is correct.
 *
 * @see K_MSGQ_DEFINE()
 */
ZTEST(msgq_api, test_define_public)
{
	zassert_equal(k_msgq_num_free_get(&kmsgq_public), MSGQ_LEN);
	zassert_equal(k_msgq_num_used_get(&kmsgq_public), 0);
	zassert_equal(kmsgq_public.msg_size, MSG_SIZE);
}

/**
 * @brief Test static-scope message queue defined at compile time
 *
 * @details
 * - Verify that the free count is equal to the queue length.
 * - Verify that the used count equals 0.
 * - Verify that the message size is correct.
 *
 * @see K_MSGQ_DEFINE_STATIC()
 */
ZTEST(msgq_api, test_define_static)
{
	zassert_equal(k_msgq_num_free_get(&kmsgq_static), MSGQ_LEN);
	zassert_equal(k_msgq_num_used_get(&kmsgq_static), 0);
	zassert_equal(kmsgq_static.msg_size, MSG_SIZE);
}

/**
 * @brief Test public-scope, typed message queue defined at compile time
 *
 * @details
 * - Verify that the free count is equal to the queue length.
 * - Verify that the used count equals 0.
 * - Verify that the message size is correct.
 *
 * @see K_MSGQ_DEFINE_TYPE()
 */
ZTEST(msgq_api, test_define_type)
{
	zassert_equal(k_msgq_num_free_get(&kmsgq_type), MSGQ_LEN);
	zassert_equal(k_msgq_num_used_get(&kmsgq_type), 0);
	zassert_equal(kmsgq_type.msg_size, sizeof(struct typed_msgq_msg));
}

/**
 * @brief Test static-scope, typed message queue defined at compile time
 *
 * @details
 * - Verify that the free count is equal to the queue length.
 * - Verify that the used count equals 0.
 * - Verify that the message size is correct.
 *
 * @see K_MSGQ_DEFINE_STATIC_TYPE()
 */
ZTEST(msgq_api, test_define_static_type)
{
	zassert_equal(k_msgq_num_free_get(&kmsgq_static_type), MSGQ_LEN);
	zassert_equal(k_msgq_num_used_get(&kmsgq_static_type), 0);
	zassert_equal(kmsgq_static_type.msg_size, sizeof(struct typed_msgq_msg));
}
