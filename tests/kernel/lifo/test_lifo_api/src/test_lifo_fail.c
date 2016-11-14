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
 * @addtogroup t_lifo_api
 * @{
 * @defgroup t_lifo_get_fail test_lifo_get_fail
 * @brief TestPurpose: verify zephyr lifo_get when no data
 * - API coverage
 *   -# k_lifo_init
 *   -# k_lifo_get [TIMEOUT|K_NO_WAIT]
 * @}
 */

#include "test_lifo.h"

#define TIMEOUT 100

/*test cases*/
void test_lifo_get_fail(void *p1, void *p2, void *p3)
{
	struct k_lifo lifo;

	k_lifo_init(&lifo);
	/**TESTPOINT: lifo get returns NULL*/
	assert_is_null(k_lifo_get(&lifo, K_NO_WAIT), NULL);
	assert_is_null(k_lifo_get(&lifo, TIMEOUT), NULL);
}

