/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_mpool.h"

extern struct k_mem_pool kmpool;

/**
 * @brief Test exern define
 *
 * If the pool is to be accessed outside the module where it is
 * defined, it can be declared via @code extern struct k_mem_pool <name>
 * @endcode
 *
 * @see k_mem_pool_alloc(), k_mem_pool_free()
 */
void test_mpool_kdefine_extern(void)
{
	tmpool_alloc_free(NULL);
}
