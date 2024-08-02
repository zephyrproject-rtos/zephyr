/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_mslab.h"

extern struct k_mem_slab kmslab;

/*test cases*/
/**
 * @brief Test mem_slab access outside the module
 *
 * @details  The memory slab can be accessed outside
 * the module where it is defined using:
 * @code extern struct k_mem_slab <name>; @endcode
 *
 * @ingroup kernel_memory_slab_tests
 */
ZTEST(mslab_api, test_mslab_kdefine_extern)
{
	tmslab_alloc_free(&kmslab);
}
