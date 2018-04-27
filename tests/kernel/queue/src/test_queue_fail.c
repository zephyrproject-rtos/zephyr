/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_queue_api
 * @{
 * @defgroup t_queue_get_fail test_queue_get_fail
 * @brief TestPurpose: verify zephyr queue_get when no data
 * - API coverage
 *   -# k_queue_init
 *   -# k_queue_get
 * @}
 */

#include "test_queue.h"

#define TIMEOUT 100

/*test cases*/
void test_queue_get_fail(void)
{
	struct k_queue queue;

	k_queue_init(&queue);
	/**TESTPOINT: queue get returns NULL*/
	zassert_is_null(k_queue_get(&queue, K_NO_WAIT), NULL);
	zassert_is_null(k_queue_get(&queue, TIMEOUT), NULL);
}

