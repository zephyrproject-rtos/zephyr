/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_fifo_api
 * @{
 * @defgroup t_fifo_get_fail test_fifo_get_fail
 * @brief TestPurpose: verify zephyr fifo_get when no data
 * - API coverage
 *   -# k_fifo_init
 *   -# k_fifo_get
 * @}
 */

#include "test_fifo.h"

#define TIMEOUT 100

/*test cases*/
void test_fifo_get_fail(void *p1, void *p2, void *p3)
{
	struct k_fifo fifo;

	k_fifo_init(&fifo);
	/**TESTPOINT: fifo get returns NULL*/
	zassert_is_null(k_fifo_get(&fifo, K_NO_WAIT), NULL);
	zassert_is_null(k_fifo_get(&fifo, TIMEOUT), NULL);
}

