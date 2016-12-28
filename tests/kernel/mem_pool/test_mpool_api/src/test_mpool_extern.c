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

#include <ztest.h>
#include "test_mpool.h"

/**
 * TESTPOINT: If the pool is to be accessed outside the module where it is
 * defined, it can be declared via @code extern struct k_mem_pool <name>
 * @endcode
 */
extern struct k_mem_pool kmpool;

/*test cases*/
void test_mpool_kdefine_extern(void)
{
	tmpool_alloc_free(NULL);
}
