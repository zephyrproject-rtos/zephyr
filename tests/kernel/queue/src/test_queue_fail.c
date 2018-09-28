/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_queue.h"

#define TIMEOUT 100

/*test cases*/
/**
 * @brief Test k_queue_get() failure scenario
 * @ingroup kernel_queue_tests
 * @see k_queue_get()
 */
void test_queue_get_fail(void)
{
	struct k_queue queue;

	k_queue_init(&queue);
	/**TESTPOINT: queue get returns NULL*/
	zassert_is_null(k_queue_get(&queue, K_NO_WAIT), NULL);
	zassert_is_null(k_queue_get(&queue, TIMEOUT), NULL);
}

