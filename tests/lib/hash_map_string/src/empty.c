/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

#define KEY_1   "first_key"
#define VALUE_1 1

ZTEST(hash_map_string, test_empty)
{
	/* test size 0 */
	zassert_true(sys_hashmap_is_empty(&map));

	/* test size 1 */
	zassume_equal(1, string_map_insert(&map, KEY_1, VALUE_1, NULL, NULL));
	zassert_false(sys_hashmap_is_empty(&map));
}
