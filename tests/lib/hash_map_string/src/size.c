/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

#define KEY_1   "first_key"
#define VALUE_1 1
#define KEY_2   "second_key"
#define VALUE_2 2

ZTEST(hash_map_string, test_size)
{
	zassert_equal(0, sys_hashmap_size(&map));

	zassume_equal(1, string_map_insert(&map, KEY_1, VALUE_1, NULL, NULL));
	zassert_equal(1, sys_hashmap_size(&map));

	zassume_equal(1, string_map_insert(&map, KEY_2, VALUE_2, NULL, NULL));
	zassert_equal(2, sys_hashmap_size(&map));
}
