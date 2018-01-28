/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_msgq_api
 * @{
 * @defgroup t_msgq_fail test_msgq_fail
 * @brief TestPurpose: verify zephyr msgq return code under negative tests
 * @}
 */


#include "test_msgq.h"

static char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static u32_t data[MSGQ_LEN] = { MSG0, MSG1 };
extern struct k_msgq msgq;

/*test cases*/
void test_msgq_put_fail(void *p1, void *p2, void *p3)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);

	int ret = k_msgq_put(&msgq, (void *)&data[0], K_NO_WAIT);

	zassert_false(ret, NULL);
	ret = k_msgq_put(&msgq, (void *)&data[0], K_NO_WAIT);
	zassert_false(ret, NULL);
	/**TESTPOINT: msgq put returns -ENOMSG*/
	ret = k_msgq_put(&msgq, (void *)&data[1], K_NO_WAIT);
	zassert_equal(ret, -ENOMSG, NULL);
	/**TESTPOINT: msgq put returns -EAGAIN*/
	ret = k_msgq_put(&msgq, (void *)&data[0], TIMEOUT);
	zassert_equal(ret, -EAGAIN, NULL);

	k_msgq_purge(&msgq);
}

void test_msgq_get_fail(void *p1, void *p2, void *p3)
{
	u32_t rx_data;

	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	/**TESTPOINT: msgq get returns -ENOMSG*/
	int ret = k_msgq_get(&msgq, &rx_data, K_NO_WAIT);

	zassert_equal(ret, -ENOMSG, NULL);
	/**TESTPOINT: msgq get returns -EAGAIN*/
	ret = k_msgq_get(&msgq, &rx_data, TIMEOUT);
	zassert_equal(ret, -EAGAIN, NULL);
}
