/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_lifo.h"

#define TIMEOUT K_MSEC(100)

/*test cases*/
/**
 * @addtogroup tests_kernel_lifo
 * @{
 */

/**
 * @brief Verify k_lifo_get() on an empty LIFO returns NULL.
 *
 * @details
 * When no data is queued, k_lifo_get() must return NULL rather than block
 * indefinitely or return stale data. The contract is checked both with
 * K_NO_WAIT (immediate) and with a finite timeout that is allowed to expire.
 *
 * Test steps:
 * - Initialize an empty LIFO.
 * - Call k_lifo_get() with K_NO_WAIT and verify it returns NULL.
 * - Call k_lifo_get() with a finite timeout and verify it returns NULL.
 *
 * Expected result:
 * - Both k_lifo_get() calls return NULL.
 *
 * @see k_lifo_get()
 * @verifies ZEP-SRS-23-4
 */
ZTEST(lifo_fail, test_lifo_get_fail)
{
	static struct k_lifo lifo;

	k_lifo_init(&lifo);
	/**TESTPOINT: lifo get returns NULL*/
	zassert_is_null(k_lifo_get(&lifo, K_NO_WAIT), NULL);
	zassert_is_null(k_lifo_get(&lifo, TIMEOUT), NULL);
}

/**
 * @}
 */

ZTEST_SUITE(lifo_fail, NULL, NULL, NULL, NULL, NULL);
