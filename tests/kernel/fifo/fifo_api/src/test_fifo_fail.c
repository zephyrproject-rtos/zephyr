/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fifo.h"

#define TIMEOUT K_MSEC(100)

/**
 * @addtogroup tests_kernel_fifo
 * @{
 */

/**
 * @brief Verify k_fifo_get() on an empty FIFO returns NULL.
 *
 * @details
 * When no data is queued, k_fifo_get() must return NULL rather than block
 * indefinitely or return stale data. The contract is checked both with
 * K_NO_WAIT (immediate) and with a finite timeout that is allowed to expire.
 *
 * Test steps:
 * - Initialize an empty FIFO.
 * - Call k_fifo_get() with K_NO_WAIT and verify it returns NULL.
 * - Call k_fifo_get() with a finite timeout and verify it returns NULL.
 *
 * Expected result:
 * - Both k_fifo_get() calls return NULL.
 *
 * @see k_fifo_get()
 */
ZTEST(fifo_api, test_fifo_get_fail)
{
	static struct k_fifo fifo;

	k_fifo_init(&fifo);
	/**TESTPOINT: fifo get returns NULL*/
	zassert_is_null(k_fifo_get(&fifo, K_NO_WAIT), NULL);
	zassert_is_null(k_fifo_get(&fifo, TIMEOUT), NULL);
}
/**
 * @}
 */
