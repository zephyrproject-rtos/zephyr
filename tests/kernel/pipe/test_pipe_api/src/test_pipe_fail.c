/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_pipe_api
 * @{
 * @defgroup t_pipe_fail_get_put test_pipe_fail_get_put
 * @brief TestPurpose: verify zephyr pipe get/put under different condition
 * - API coverage
 *   -# k_pipe_get [TIMEOUT K_NO_WAIT]
 *   -# k_pipe_put [K_FOREVER TIMEOUT K_NO_WAIT]
 * @}
 */

#include <ztest.h>

#define TIMEOUT 100
#define PIPE_LEN 8

static unsigned char __aligned(4) data[] = "abcd1234";

/*test cases*/
void test_pipe_put_fail(void *p1, void *p2, void *p3)
{
	struct k_pipe pipe;
	size_t wt_byte = 0;

	k_pipe_init(&pipe, data, PIPE_LEN);
	zassert_false(k_pipe_put(&pipe, data, PIPE_LEN, &wt_byte,
				1, K_FOREVER), NULL);
	/**TESTPOINT: pipe put returns -EIO*/
	zassert_equal(k_pipe_put(&pipe, data, PIPE_LEN, &wt_byte,
				1, K_NO_WAIT), -EIO, NULL);
	zassert_false(wt_byte, NULL);
	/**TESTPOINT: pipe put returns -EAGAIN*/
	zassert_equal(k_pipe_put(&pipe, data, PIPE_LEN, &wt_byte,
				1, TIMEOUT), -EAGAIN, NULL);
	zassert_true(wt_byte < 1, NULL);
}

void test_pipe_get_fail(void *p1, void *p2, void *p3)
{
	struct k_pipe pipe;
	unsigned char rx_data[PIPE_LEN];
	size_t rd_byte = 0;

	k_pipe_init(&pipe, data, PIPE_LEN);
	/**TESTPOINT: pipe put returns -EIO*/
	zassert_equal(k_pipe_get(&pipe, rx_data, PIPE_LEN, &rd_byte, 1,
				K_NO_WAIT), -EIO, NULL);
	zassert_false(rd_byte, NULL);
	/**TESTPOINT: pipe put returns -EAGAIN*/
	zassert_equal(k_pipe_get(&pipe, rx_data, PIPE_LEN, &rd_byte, 1,
				TIMEOUT), -EAGAIN, NULL);
	zassert_true(rd_byte < 1, NULL);
}
