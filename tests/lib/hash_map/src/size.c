/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_size)
{
	zassert_equal(0, sys_hashmap_size(&map));

	zassume_equal(1, sys_hashmap_insert(&map, 1, 1, NULL));
	zassert_equal(1, sys_hashmap_size(&map));

	zassume_equal(1, sys_hashmap_insert(&map, 2, 2, NULL));
	zassert_equal(2, sys_hashmap_size(&map));
}
