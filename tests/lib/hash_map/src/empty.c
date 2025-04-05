/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_empty)
{
	/* test size 0 */
	zassert_true(sys_hashmap_is_empty(&map));

	/* test size 1 */
	zassume_equal(1, INSERT_FUNC(&map, 1, 1, NULL, NULL));
	zassert_false(sys_hashmap_is_empty(&map));
}
