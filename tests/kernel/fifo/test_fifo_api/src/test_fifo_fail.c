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
	assert_is_null(k_fifo_get(&fifo, K_NO_WAIT), NULL);
	assert_is_null(k_fifo_get(&fifo, TIMEOUT), NULL);
}

