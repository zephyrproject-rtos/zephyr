/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_insert_no_replacement)
{
	zassert_true(sys_hashmap_is_empty(&map));

	zassert_equal(1, sys_hashmap_insert(&map, 1, 1, NULL));
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(sys_hashmap_contains_key(&map, 1));

	zassert_equal(1, sys_hashmap_insert(&map, 2, 2, NULL));
	zassert_equal(2, sys_hashmap_size(&map));
	zassert_true(sys_hashmap_contains_key(&map, 2));
}

ZTEST(hash_map, test_insert_replacement)
{
	uint64_t old_value;

	zassert_true(sys_hashmap_is_empty(&map));

	zassert_equal(1, sys_hashmap_insert(&map, 1, 1, NULL));
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(sys_hashmap_contains_key(&map, 1));

	old_value = 0x42;
	zassert_equal(0, sys_hashmap_insert(&map, 1, 2, &old_value));
	zassert_equal(1, old_value);
	zassert_equal(1, sys_hashmap_size(&map));
	zassert_true(sys_hashmap_contains_key(&map, 1));
}

ZTEST(hash_map, test_insert_many)
{
	int ret;

	zassert_true(sys_hashmap_is_empty(&map));

	for (size_t i = 0; i < MANY; ++i) {
		ret = sys_hashmap_insert(&map, i, i, NULL);
		zassert_equal(1, ret, "failed to insert (%zu, %zu): %d", i, i, ret);
		zassert_equal(i + 1, sys_hashmap_size(&map));
	}

	for (size_t i = 0; i < MANY; ++i) {
		zassert_true(sys_hashmap_contains_key(&map, i));
	}
}
