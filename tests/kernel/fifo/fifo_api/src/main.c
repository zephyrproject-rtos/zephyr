/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Tests for the FIFO kernel object
 *
 * Verify the Zephyr FIFO APIs across thread and ISR contexts, including data
 * passing, emptiness queries, cancellation, timeouts and queueing order.
 *
 * - API coverage
 *   -# k_fifo_init() K_FIFO_DEFINE()
 *   -# k_fifo_put() k_fifo_put_list() k_fifo_put_slist()
 *   -# k_fifo_get() k_fifo_is_empty() k_fifo_cancel_wait()
 *
 * @defgroup tests_kernel_fifo FIFO tests
 * @ingroup all_tests
 * @{
 * @}
 */

#include <zephyr/ztest.h>

ZTEST_SUITE(fifo_api, NULL, NULL, NULL, NULL, NULL);

ZTEST_SUITE(fifo_api_1cpu, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
