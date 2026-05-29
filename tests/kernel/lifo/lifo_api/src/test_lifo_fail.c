/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_lifo.h"

#define TIMEOUT K_MSEC(100)

/*test cases*/
/**
 * @addtogroup kernel_lifo_tests
 * @{
 */

/**
 * @brief Test LIFO get fail
 * @details verify zephyr k_lifo_get, it returns NULL
 * when there is no data to read
 * @see k_lifo_init(), k_lifo_get()
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
