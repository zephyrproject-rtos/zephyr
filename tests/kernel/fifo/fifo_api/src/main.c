/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Tests for the FIFO kernel object
 *
 * Verify zephyr fifo apis under different context
 *
 * - API coverage
 *   -# k_fifo_init K_FIFO_DEFINE
 *   -# k_fifo_put k_fifo_put_list k_fifo_put_slist
 *   -# k_fifo_get *
 *
 * @defgroup kernel_fifo_tests FIFOs
 * @ingroup all_tests
 * @{
 * @}
 */

#include <zephyr/ztest.h>

ZTEST_SUITE(fifo_api, NULL, NULL, NULL, NULL, NULL);

ZTEST_SUITE(fifo_api_1cpu, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
