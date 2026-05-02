/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/hash_map.h>

#include "_main.h"

ZTEST(hash_map, test_get_true)
{
	int ret;
	uint64_t value = 0x42;

	zassert_true(sys_hashmap_is_empty(&map));
	zassert_equal(1, sys_hashmap_insert(&map, 0, 0, NULL));
	zassert_true(sys_hashmap_get(&map, 0, NULL));
	zassert_true(sys_hashmap_get(&map, 0, &value));
	zassert_equal(0, value);

	for (size_t i = 1; i < MANY; ++i) {
		ret = sys_hashmap_insert(&map, i, i, NULL);
		zassert_equal(1, ret, "failed to insert (%zu, %zu): %d", i, i, ret);
	}

	for (size_t i = 0; i < MANY; ++i) {
		zassert_true(sys_hashmap_get(&map, i, NULL));
	}
}

ZTEST(hash_map, test_get_false)
{
	int ret;
	uint64_t value = 0x42;

	zassert_true(sys_hashmap_is_empty(&map));

	zassert_false(sys_hashmap_get(&map, 73, &value));
	zassert_equal(value, 0x42);

	for (size_t i = 0; i < MANY; ++i) {
		ret = sys_hashmap_insert(&map, i, i, NULL);
		zassert_equal(1, ret, "failed to insert (%zu, %zu): %d", i, i, ret);
	}

	zassert_false(sys_hashmap_get(&map, 0x4242424242424242ULL, NULL));
}
