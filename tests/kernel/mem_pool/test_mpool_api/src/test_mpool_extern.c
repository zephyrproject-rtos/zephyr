/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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
