/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup t_pipe_api
 * @{
 * @defgroup t_pipe_fail_get_put test_pipe_fail_get_put
 * @brief TestPurpose: verify zephyr pipe get/put under differnt condition
 * - API coverage
 *   -# k_pipe_get [TIMEOUT K_NO_WAIT]
 *   -# k_pipe_put [K_FOREVER TIMEOUT K_NO_WAIT]
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>

#define TIMEOUT 100
#define PIPE_LEN 8

static unsigned char __aligned(4) data[] = "abcd1234";

/*test cases*/
void test_pipe_put_fail(void *p1, void *p2, void *p3)
{
	struct k_pipe pipe;
	size_t wt_byte = 0;

	k_pipe_init(&pipe, data, PIPE_LEN);
	assert_false(k_pipe_put(&pipe, data, PIPE_LEN, &wt_byte,
				1, K_FOREVER), NULL);
	/**TESTPOINT: pipe put returns -EIO*/
	assert_equal(k_pipe_put(&pipe, data, PIPE_LEN, &wt_byte,
				1, K_NO_WAIT), -EIO, NULL);
	assert_false(wt_byte, NULL);
	/**TESTPOINT: pipe put returns -EAGAIN*/
	assert_equal(k_pipe_put(&pipe, data, PIPE_LEN, &wt_byte,
				1, TIMEOUT), -EAGAIN, NULL);
	assert_true(wt_byte < 1, NULL);
}

void test_pipe_get_fail(void *p1, void *p2, void *p3)
{
	struct k_pipe pipe;
	unsigned char rx_data[PIPE_LEN];
	size_t rd_byte = 0;

	k_pipe_init(&pipe, data, PIPE_LEN);
	/**TESTPOINT: pipe put returns -EIO*/
	assert_equal(k_pipe_get(&pipe, rx_data, PIPE_LEN, &rd_byte, 1,
				K_NO_WAIT), -EIO, NULL);
	assert_false(rd_byte, NULL);
	/**TESTPOINT: pipe put returns -EAGAIN*/
	assert_equal(k_pipe_get(&pipe, rx_data, PIPE_LEN, &rd_byte, 1,
				TIMEOUT), -EAGAIN, NULL);
	assert_true(rd_byte < 1, NULL);
}
